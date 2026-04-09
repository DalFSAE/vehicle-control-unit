#include <stddef.h>

#include "vcu_io.h"
#include "vehicle_state.h"
#include "board_outputs.h"
#include "output_control.h"
#include "torque_output.h"
#include "dio.h"

#define MOCK_IO true

void vcu_read_inputs(VcuInputs *in) {
    if (in == NULL) return;

#if MOCK_IO
    // todo: connect to external interface. Possibly python over USB
    in->throttle_request    = 0.5;
    in->brake_pressed       = true;
    in->fault_flags         = g_vcu.fault_flags;
    in->rtd_button          = g_vcu.rtd_button;
    in->fwrd_switch         = true;
    in->ts_active           = false;
#else
    in->throttle_request    = g_vehicle.throttle_request;
    in->brake_pressed       = g_vehicle.brake_pressed;
    in->fault_flags         = g_vehicle.fault_flags;
    in->rtd_button          = g_vehicle.rtd_button;
    in->fwrd_switch         = g_vehicle.fwrd_switch;
    in->ts_active           = g_can.ts_active; // todo: get from CAN bus
#endif
}

void vcu_apply_outputs(const VcuOutputs *out) {
    // Relays
    out->relay_always_on ? board_output_enable(OUTPUT_ALWAYS_ON) : board_output_disable(OUTPUT_ALWAYS_ON);
    out->relay_inverter ? board_output_enable(OUTPUT_INVERTER) : board_output_disable(OUTPUT_INVERTER);
    out->brake_light ? board_output_enable(OUTPUT_BRAKE_LIGHT) : board_output_disable(OUTPUT_BRAKE_LIGHT);

    // Digital outputs
    dio_write(CAN_WATCHDOG, out->can_watchdog);
    dio_write(TSSI_EN, out->tssi_en);
    dio_write(MC_BRAKE_SW, out->mc_brake_sw);

    // Motor direction and torque
    mc_set_direction(out->motor_direction);
    torque_output_update(out->throttle_request, out->throttle_enabled);

    // Buzzer
    if (out->buzzer_beep_ms) {
        buzzer_beep(out->buzzer_beep_ms);
    }
    buzzer_update();
}
