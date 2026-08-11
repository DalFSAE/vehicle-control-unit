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
#include "log.h"
#include "main.h"

// Set true to trace VcuOutputs field changes over serial. Leave false in normal builds.
#define VCU_IO_DEBUG_LOG true

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

// Apply debug LED states from FSM outputs to hardware.
void vcu_apply_debug_leds(uint8_t debug_leds) {
    board_output_set(OUTPUT_DEBUG_LED4, debug_leds & 0x01);
    board_output_set(OUTPUT_DEBUG_LED5, (debug_leds >> 1) & 0x01);
    board_output_set(OUTPUT_DEBUG_LED6, (debug_leds >> 2) & 0x01);
}

#if VCU_IO_DEBUG_LOG
// Logs each VcuOutputs field that changed since the previous applied struct, old -> new.
static void log_output_changes(const VcuOutputs *out) {
    static VcuOutputs prev;
    static bool       have_prev = false;
    unsigned long     now       = (unsigned long)HAL_GetTick();

    if (!have_prev) { // print a one-time baseline snapshot, then diff from here
        prev      = *out;
        have_prev = true;
        log_printf(
            "[%8lu] IO baseline relay_always_on=%u relay_inverter=%u brake_light=%u mc_brake_sw=%u "
            "can_watchdog=%u tssi_en=%u throttle_enabled=%u sdc_open=%u buzzer_beep_ms=%lu debug_leds=%u "
            "motor_direction=%s throttle_request=%lu(/1000)\r\n",
            now, (unsigned)out->relay_always_on, (unsigned)out->relay_inverter, (unsigned)out->brake_light,
            (unsigned)out->mc_brake_sw, (unsigned)out->can_watchdog, (unsigned)out->tssi_en,
            (unsigned)out->throttle_enabled, (unsigned)out->sdc_open, (unsigned long)out->buzzer_beep_ms,
            (unsigned)out->debug_leds, out->motor_direction == MOTOR_DIR_FORWARD ? "FWD" : "REV",
            (unsigned long)(out->throttle_request * 1000.0f));
        return;
    }

    #define TRACE_U(field)                                                                 \
        if (out->field != prev.field)                                                       \
            log_printf("[%8lu] IO " #field " %lu -> %lu\r\n", now, (unsigned long)prev.field, \
                       (unsigned long)out->field);

    TRACE_U(relay_always_on);
    TRACE_U(relay_inverter);
    TRACE_U(brake_light);
    TRACE_U(mc_brake_sw);
    TRACE_U(can_watchdog);
    TRACE_U(tssi_en);
    TRACE_U(throttle_enabled);
    TRACE_U(sdc_open);
    TRACE_U(buzzer_beep_ms);
    TRACE_U(debug_leds);
    #undef TRACE_U

    if (out->motor_direction != prev.motor_direction) {
        log_printf("[%8lu] IO motor_direction %s -> %s\r\n", now,
                   prev.motor_direction == MOTOR_DIR_FORWARD ? "FWD" : "REV",
                   out->motor_direction == MOTOR_DIR_FORWARD ? "FWD" : "REV");
    }

    // throttle_request changes nearly every cycle while driving, so this is chatty.
    // Comment out if its spams the log.
    if (out->throttle_request != prev.throttle_request) {
        log_printf("[%8lu] IO throttle_request %lu -> %lu (/1000)\r\n", now,
                   (unsigned long)(prev.throttle_request * 1000.0f),
                   (unsigned long)(out->throttle_request * 1000.0f));
    }

    prev = *out;
}
#endif

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

    log_output_changes(out);

    // Relays
    out->relay_always_on ? board_output_enable(OUTPUT_ALWAYS_ON)   : board_output_disable(OUTPUT_ALWAYS_ON);
    out->relay_inverter  ? board_output_enable(OUTPUT_INVERTER)     : board_output_disable(OUTPUT_INVERTER);
    out->brake_light     ? board_output_enable(OUTPUT_BRAKE_LIGHT)  : board_output_disable(OUTPUT_BRAKE_LIGHT);

    // SDC relay: sdc_open and can_watchdog (CAN heartbeat timeout) both independently
    // request an open shutdown circuit, so either one holds it de-energized.
    bool sdc_energize = !out->sdc_open && !out->can_watchdog;
    sdc_energize ? board_output_enable(OUTPUT_SDC) : board_output_disable(OUTPUT_SDC);

    // Digital outputs
    dio_write(TSSI_EN, out->tssi_en);
    dio_write(MC_BRAKE_SW, out->mc_brake_sw);

    // Buzzer
    if (out->buzzer_beep_ms) {
        buzzer_beep(out->buzzer_beep_ms);
    }
    buzzer_update();

    // Build motor controller command and push to cache (can_task sends it).
    MotorControllerCmd_t cmd = {
        .inv_enable              = out->throttle_enabled,
        .motor_direction_forward = (out->motor_direction == MOTOR_DIR_FORWARD),
        .torque_command_nm       = out->throttle_enabled ? motor_torque(out->throttle_request) : 0.0f,
        .torque_limit_nm         = MC_TORQUE_LIMIT_NM,
        .inv_discharge           = false,
        .speed_mode_enable       = false,
    };
    motor_controller_set_cmd(&cmd);

    // Debug LEDs
    vcu_apply_debug_leds(out->debug_leds);
}
