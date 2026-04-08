#include "vcu_io.h"
#include "vehicle_state.h"
#include "board_outputs.h"
#include "io_control.h"
#include "torque_output.h"

void vcu_read_inputs(VcuInputs *in)
{
    in->throttle_request = g_vehicle.throttle_request;
    in->brake_pressed    = g_vehicle.brake_pressed;
    in->fault_flags      = g_vehicle.fault_flags;
    in->rtd_pressed      = dio_read(DASH_RTD_BUTTON);
    in->dash_switch_on   = (read_dash_switch_filtered() == SWITCH_ON_LEVEL);
}

void vcu_apply_outputs(const VcuOutputs *out)
{
    // Relays
    out->relay_always_on ? board_output_enable(OUTPUT_ALWAYS_ON)   : board_output_disable(OUTPUT_ALWAYS_ON);
    out->relay_inverter  ? board_output_enable(OUTPUT_INVERTER)    : board_output_disable(OUTPUT_INVERTER);
    out->brake_light     ? board_output_enable(OUTPUT_BRAKE_LIGHT) : board_output_disable(OUTPUT_BRAKE_LIGHT);

    // Digital outputs
    dio_write(CAN_WATCHDOG, out->can_watchdog);
    dio_write(TSSI_EN,      out->tssi_en);
    dio_write(MC_BRAKE_SW,  out->mc_brake_sw);

    // Motor direction and torque
    mc_set_direction(out->motor_direction);
    torque_output_update(out->throttle_request, out->throttle_enabled);

    // Buzzer
    if (out->buzzer_beep_ms) {
        buzzer_beep(out->buzzer_beep_ms);
    }
    buzzer_update();
}
