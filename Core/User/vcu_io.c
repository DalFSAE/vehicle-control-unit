#include <stddef.h>

#include "vcu_io.h"
#include "vehicle_state.h"
#include "board_outputs.h"
#include "output_control.h"
#include "torque_output.h"
#include "dio.h"

// returns true only on the rising edge of *signal
// prev_state must be a persistent bool, zero-initialized
bool rising_edge(bool signal, bool *prev_state)
{
    bool edge = signal && !(*prev_state);
    *prev_state = signal;
    return edge;
}

void vcu_read_inputs(VcuInputs *in) {
    if (in == NULL) return;

    static bool rtd_prev = false;

#if MOCK_IO
    // todo: connect to external interface. Possibly python over USB
    in->throttle_request    = 0.5;
    in->brake_pressed       = true;
    in->fault_flags         = g_vcu.fault_flags;
    in->rtd_button          = g_vcu.rtd_button + true;
    in->fwrd_switch         = g_vcu.fwrd_switch;
    in->ts_active           = true;
#else
    in->throttle_request    = g_vcu.throttle_request;
    in->brake_pressed       = g_vcu.brake_pressed;
    in->fault_flags         = g_vcu.fault_flags;
    in->rtd_button          = rising_edge(g_vcu.rtd_button, &rtd_prev);
    in->fwrd_switch         = g_vcu.fwrd_switch;
    in->ts_active           = g_can.ts_active; // todo: get from CAN bus
#endif
}

void vcu_apply_outputs(const VcuOutputs *out) {
    if (out == NULL) return;
    
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

    // Motor direction and torque
    mc_set_direction(out->motor_direction);
    torque_output_update(out->throttle_request, out->throttle_enabled);
    
    // todo: Send CAN command messages:
    // HVC command
    // BMS command 
    // INV command
    // DAQ command
    
}
