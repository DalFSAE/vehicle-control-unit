#include "fsm_test_helpers.h"
#include "fsm_task.h"
#include "vcu_io.h"
#include "sensor_control.h"
#include "cmsis_os2.h"

#include <string.h>

VcuInputs g_spoof = {0};

void suspend_sensor(void) { osThreadSuspend(sensor_task_get_handle()); }
void resume_sensor(void)  { osThreadResume(sensor_task_get_handle()); }


void clear_inputs(void) {
    memset(&g_spoof, 0, sizeof(g_spoof));
    vcu_spoof_inputs(&g_spoof);
}

void walk_to_neutral(void) {
    clear_inputs();
    g_spoof.fwrd_switch = true;
    g_spoof.ts_active   = true;
    vcu_spoof_inputs(&g_spoof);
    osDelay(FSM_SETTLE_MS); // allow FSM to process and update state
}

void walk_to_forward(void) {
    g_spoof.fwrd_switch   = true;
    g_spoof.brake_pressed = true;
    g_spoof.rtd_button    = true;
    vcu_spoof_inputs(&g_spoof);
    osDelay(FSM_SETTLE_MS);
    g_spoof.rtd_button = false;
    vcu_spoof_inputs(&g_spoof);
}

void ramp_throttle(float start, float end, uint32_t steps, uint32_t delta_ms) {
    for (uint32_t i = 0; i <= steps; i++) {
        g_spoof.throttle_request = start + (end - start) * (i / (float)steps);
        vcu_spoof_inputs(&g_spoof);
        osDelay(delta_ms);
    }
}
