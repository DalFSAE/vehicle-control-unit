// vcu_io.c
#include <stddef.h>
#include <string.h>

#include "vcu_io.h"
#include "board_outputs.h"
#include "output_control.h"
#include "motor_controller.h"
#include "sensor_control.h"
#include "input_control.h"
#include "dio.h"
#include "dash.h"

// HIL spoof
static bool s_spoof_active = false;
static VcuInputs s_spoof   = {0};

void vcu_spoof_inputs(const VcuInputs *spoof) {
    if (spoof == NULL) {
        return;
    }
    s_spoof        = *spoof;
    s_spoof_active = true;
}

void vcu_clear_spoof(void) {
    s_spoof_active = false;
}

void vcu_fault_inject(uint32_t flags) {
    VcuInputs spoofed   = s_spoof_active ? s_spoof : (VcuInputs){0};
    spoofed.fault_flags = flags;
    vcu_spoof_inputs(&spoofed);
}

// Edge detection helper (persistent prev state per call site)
static bool rising_edge(bool signal, bool *prev) {
    bool edge = signal && !(*prev);
    *prev     = signal;
    return edge;
}

// Gather inputs from hardware/sensors into *in. Called by main loop.
void vcu_gather_inputs(VcuInputs *in) {
    if (in == NULL) {
        return;
    }

    if (s_spoof_active) {
        *in = s_spoof;
        return;
    }

    static bool rtd_prev = false;
    bool rtd_raw         = read_pcb_user_button() || read_ready_to_drive_button();

    in->throttle_request = sensor_get_throttle();
    in->brake_pressed    = sensor_get_brake();
    in->fault_flags      = sensor_get_fault_flags();
    if (mc_has_timeout()) {
        in->fault_flags |= FAULT_CAN_TIMEOUT;
    }
    in->rtd_button  = rising_edge(rtd_raw, &rtd_prev);
    in->fwrd_switch = read_forward_switch();
    in->rvrs_switch = false; // no reverse switch wired yet
    in->ts_active   = mc_is_ready();
}

// Apply outputs to hardware
void vcu_apply_outputs(const VcuOutputs *out) {
    if (out == NULL) {
        return;
    }

    // Relays
    out->relay_always_on ? board_output_enable(OUTPUT_ALWAYS_ON) : board_output_disable(OUTPUT_ALWAYS_ON);
    out->relay_inverter ? board_output_enable(OUTPUT_INVERTER) : board_output_disable(OUTPUT_INVERTER);
    out->brake_light ? board_output_enable(OUTPUT_BRAKE_LIGHT) : board_output_disable(OUTPUT_BRAKE_LIGHT);

    // Digital outputs
    dio_write(CAN_WATCHDOG, out->can_watchdog);
    dio_write(TSSI_EN, out->tssi_en);
    dio_write(MC_BRAKE_SW, out->mc_brake_sw);

    // Buzzer
    if (out->buzzer_beep_ms) {
        buzzer_beep(out->buzzer_beep_ms);
    }
    buzzer_update();

    // Motor direction
    mc_set_direction(out->motor_direction);

    // Build motor controller command and push to cache (can_task sends it).
    MotorControllerCmd_t cmd = {
        .inv_enable              = out->throttle_enabled,
        .motor_direction_forward = (out->motor_direction == MOTOR_DIR_FORWARD),
        .torque_command_nm       = out->throttle_enabled ? out->throttle_request * MC_TORQUE_MAX_NM : 0.0f,
        .torque_limit_nm         = MC_TORQUE_LIMIT_NM,
        .inv_discharge           = false,
        .speed_mode_enable       = false,
    };
    motor_controller_set_cmd(&cmd);
}
