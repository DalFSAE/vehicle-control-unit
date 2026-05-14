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

extern CAN_HandleTypeDef hcan1;

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

// Lifecycle

void motor_controller_init(void) {
    s_cmd_mutex = osMutexNew(NULL);
    can_bus_init(&hcan1, CAN_MODE_NORMAL);
    LOG_EVENT(LOG_LEVEL_INFO, EVT_BOOT, 0u, 0u);
}

// Command cache
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

// CAN TX
void can_tx_send_inverter_cmd(const MotorControllerCmd_t *cmd) {
    if (cmd == NULL)
        return;

    struct can0_powertrain_m192_command_message_t msg;
    can0_powertrain_m192_command_message_init(&msg);
    msg.vcu_inv_torque_command          
        = can0_powertrain_m192_command_message_vcu_inv_torque_command_encode(cmd->torque_command_nm);
    msg.vcu_inv_torque_limit_command    
        = can0_powertrain_m192_command_message_vcu_inv_torque_limit_command_encode(cmd->torque_limit_nm);
    msg.vcu_inv_speed_command           
        = can0_powertrain_m192_command_message_vcu_inv_speed_command_encode(cmd->speed_command_rpm);
    msg.vcu_inv_inverter_enable     = cmd->inv_enable ? 1u : 0u;
    msg.vcu_inv_inverter_discharge  = cmd->inv_discharge ? 1u : 0u;
    msg.vcu_inv_speed_mode_enable   = cmd->speed_mode_enable ? 1u : 0u;
    msg.vcu_inv_direction_command   = cmd->motor_direction_forward ? 1u : 0u;
    msg.vcu_inv_rolling_counter     = cmd->rolling_counter;

    uint8_t buf[CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_LENGTH];
    can0_powertrain_m192_command_message_pack(buf, &msg, sizeof(buf));
    can_bus_transmit(CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_FRAME_ID, buf, sizeof(buf));
}

// CAN RX called from ISR context via can_bus dispatch.
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

// State getters

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
