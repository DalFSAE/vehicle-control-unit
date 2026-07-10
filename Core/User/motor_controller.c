#define LOG_MODULE LOG_SRC_MC
#include "motor_controller.h"
#include "can_bus.h"
#include "log.h"
#include "can0_powertrain.h"
#include "node.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

#include <string.h>
#include <stddef.h>
#include <math.h>


// Private inverter state written from ISR, read from task context.
static struct {
    volatile uint8_t  vsm_state;
    volatile float    torque_fb_nm;
    volatile float    torque_cmd_nm;
    volatile uint32_t post_fault;
    volatile uint32_t run_fault;
    volatile uint32_t last_rx_tick_ms;
} s_inv = {0};

// Command cache written by FSM task, read by can_task.
static MotorControllerCmd_t s_cmd = {0};
static osMutexId_t          s_cmd_mutex = NULL;

// Configurable torque parameters
static motor_torque_config_t config;

// ---------------------------------------------------------------------------
// Motor controller state getters
// ---------------------------------------------------------------------------

void motor_controller_init(void) {
    s_cmd_mutex = osMutexNew(NULL);
    LOG_EVENT(LOG_LEVEL_INFO, EVT_BOOT, 0u, 0u);
}

void motor_torque_init(motor_torque_config_t cfg) {
    config = cfg;
}

bool mc_is_ready(void) {
    return s_inv.vsm_state >= MC_VSM_READY;
}

bool mc_has_timeout(void) {
    return (HAL_GetTick() - s_inv.last_rx_tick_ms) > MC_HEARTBEAT_TIMEOUT_MS;
}

uint32_t mc_fault_bitmap(void) {
    return s_inv.post_fault | s_inv.run_fault;
}

uint8_t mc_vsm_state(void) {
    return s_inv.vsm_state;
}


// ---------------------------------------------------------------------------
// Command interface
// ---------------------------------------------------------------------------

void motor_controller_set_cmd(const MotorControllerCmd_t *cmd) {
    if (cmd == NULL || s_cmd_mutex == NULL)
        return;
    osMutexAcquire(s_cmd_mutex, osWaitForever);
    s_cmd = *cmd;
    osMutexRelease(s_cmd_mutex);
}

void motor_controller_get_cmd(MotorControllerCmd_t *out) {
    if (out == NULL || s_cmd_mutex == NULL)
        return;
    osMutexAcquire(s_cmd_mutex, osWaitForever);
    *out = s_cmd;
    osMutexRelease(s_cmd_mutex);
}

// Inverter CAN TX called from can_task context
void can_tx_send_inverter_cmd(const MotorControllerCmd_t *cmd) {
    if (cmd == NULL) {
        return;
    } 
    
    struct can0_powertrain_m192_command_message_t msg;
    can0_powertrain_m192_command_message_init(&msg);
    msg.vcu_inv_torque_command       = can0_powertrain_m192_command_message_vcu_inv_torque_command_encode(cmd->torque_command_nm);
    msg.vcu_inv_torque_limit_command = can0_powertrain_m192_command_message_vcu_inv_torque_limit_command_encode(cmd->torque_limit_nm);
    msg.vcu_inv_speed_command        = can0_powertrain_m192_command_message_vcu_inv_speed_command_encode(cmd->speed_command_rpm);
    msg.vcu_inv_inverter_enable      = cmd->inv_enable ? 1u : 0u;
    msg.vcu_inv_inverter_discharge   = cmd->inv_discharge ? 1u : 0u;
    msg.vcu_inv_speed_mode_enable    = cmd->speed_mode_enable ? 1u : 0u;
    msg.vcu_inv_direction_command    = cmd->motor_direction_forward ? 1u : 0u;
    msg.vcu_inv_rolling_counter      = cmd->rolling_counter;

    uint8_t buf[CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_LENGTH];
    can0_powertrain_m192_command_message_pack(buf, &msg, sizeof(buf));
    can_bus_transmit(CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_FRAME_ID, buf, sizeof(buf));
}

// Inverter CAN RX called from ISR context via can_bus dispatch.
void inverter_rx(uint32_t id, const uint8_t *data, size_t len) {
    switch (id) {
        case CAN0_POWERTRAIN_M170_INTERNAL_STATES_FRAME_ID: {
            struct can0_powertrain_m170_internal_states_t m;
            if (can0_powertrain_m170_internal_states_unpack(&m, data, len) == 0) {
                if (m.inv_vsm_state != s_inv.vsm_state) {
                    LOG_EVENT(LOG_LEVEL_INFO, EVT_STATE_CHANGE, s_inv.vsm_state, m.inv_vsm_state);
                    s_inv.vsm_state = m.inv_vsm_state;
                }
                s_inv.last_rx_tick_ms = HAL_GetTick();
            }
            break;
        }
        case CAN0_POWERTRAIN_M171_FAULT_CODES_FRAME_ID: {
            struct can0_powertrain_m171_fault_codes_t m;
            if (can0_powertrain_m171_fault_codes_unpack(&m, data, len) == 0) {
                uint32_t prev_faults = s_inv.post_fault | s_inv.run_fault;
                s_inv.post_fault = ((uint32_t)m.inv_post_fault_hi << 16) | m.inv_post_fault_lo;
                s_inv.run_fault  = ((uint32_t)m.inv_run_fault_hi  << 16) | m.inv_run_fault_lo;
                uint32_t new_faults = s_inv.post_fault | s_inv.run_fault;
                if (new_faults != prev_faults) {
                    LogEventId_t evt = (new_faults != 0u) ? EVT_FAULT_SET : EVT_FAULT_CLEAR;
                    LogLevel_t   lvl = (new_faults != 0u) ? LOG_LEVEL_ERROR : LOG_LEVEL_INFO;
                    LOG_EVENT(lvl, evt, prev_faults, new_faults);
                }
            }
            break;
        }
        case CAN0_POWERTRAIN_M172_TORQUE_AND_TIMER_INFO_FRAME_ID: {
            struct can0_powertrain_m172_torque_and_timer_info_t m;
            if (can0_powertrain_m172_torque_and_timer_info_unpack(&m, data, len) == 0) {
                s_inv.torque_cmd_nm = (float)can0_powertrain_m172_torque_and_timer_info_inv_commanded_torque_decode(m.inv_commanded_torque);
                s_inv.torque_fb_nm  = (float)can0_powertrain_m172_torque_and_timer_info_inv_torque_feedback_decode(m.inv_torque_feedback);
            }
            break;
        }
        default: break;
    }
}

// Node entry referenced by can_bus dispatch table.
const CanNode_t inverter_node = {
    .name = "inverter",
    .rx = inverter_rx,
};

// ---------------------------------------------------------------------------
// Torque processing
// ---------------------------------------------------------------------------

static float clampf(float value, float lo, float hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

// this function returns an enum value defined in the header file
static torque_state_t determine_pedal_state(float value) {
    if (value < config.pedal_lo) {
        // Below the resting deadzone: this is the pedal's normal at-rest
        // position, not a fault. Treat the same as coast (0 Nm).
        return TORQUE_STATE_DEADZONE_LO;
    } else if (value <= config.accel_min) {
        return TORQUE_STATE_REGEN_FULL;
    } else if (value <= config.coast_lo) {
        return TORQUE_STATE_REGEN_RAMP;
    } else if (value <= config.coast_hi) {
        return TORQUE_STATE_COAST;
    } else if (value <= config.accel_max) {
        return TORQUE_STATE_ACCEL_RAMP;
    } else if (value <= config.pedal_hi) {
        return TORQUE_STATE_ACCEL_FULL;
    } else {
        // Above pedal_hi: pedal is fully (or over-) pressed. Genuine
        // open/short faults are caught upstream by sensor_out_of_range()
        // (pedal_logic.c) which uses a wider [-0.1, 1.1] band; within
        // [pedal_hi, 1.0] this is just "pedal floored," so keep commanding
        // full torque rather than cliff to 0 Nm.
        return TORQUE_STATE_ACCEL_FULL;
    }
}

// these functions are used to calculate the torque output based on the current state of the pedal position
static float state_deadzone_lo(void) {
    return 0.0f;
}

static float state_regen_full(void) {
    return config.regen_torque_limit;
}

static float state_regen_ramp(float value) {
    return (1 - (value - config.accel_min) / (config.coast_lo - config.accel_min)) * config.regen_torque_limit;
}

static float state_coast(void) {
    return 0.0f;
}

static float state_accel_ramp(float value) {
    return (value - config.coast_hi) / (config.accel_max - config.coast_hi) * config.motor_torque_limit;
}

static float state_accel_full(void) {
    return config.motor_torque_limit;
}

// Main torque calculation function. Called from vcu_apply_outputs() to convert normalized pedal position to torque command.
float motor_torque(float pedal_pos) {
    // Clamp to [0, 1] to avoid negative torque or exceeding configured limits.
    pedal_pos = clampf(pedal_pos, 0.0f, 1.0f);

    // Determine the torque state based on the pedal position and compute the corresponding torque.
    torque_state_t state = determine_pedal_state(pedal_pos);
    float torque_nm;
    switch (state) {
        case TORQUE_STATE_DEADZONE_LO: torque_nm = state_deadzone_lo();          break;
        case TORQUE_STATE_REGEN_FULL:  torque_nm = state_regen_full();           break;
        case TORQUE_STATE_REGEN_RAMP:  torque_nm = state_regen_ramp(pedal_pos);  break;
        case TORQUE_STATE_COAST:       torque_nm = state_coast();                break;
        case TORQUE_STATE_ACCEL_RAMP:  torque_nm = state_accel_ramp(pedal_pos);  break;
        case TORQUE_STATE_ACCEL_FULL:  torque_nm = state_accel_full();           break;
        default:                       torque_nm = state_deadzone_lo();          break;
    }

    // Regen is negative torque, accel is positive; clamp to the configured
    // envelope so a bad config or float rounding can't exceed either limit.
    float regen_floor = -fabsf(config.regen_torque_limit);
    float accel_ceil  =  fabsf(config.motor_torque_limit);
    return clampf(torque_nm, regen_floor, accel_ceil);
}
