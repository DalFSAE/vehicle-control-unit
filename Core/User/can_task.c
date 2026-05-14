#define LOG_MODULE LOG_SRC_CAN
#include "log.h"

#include "can_task.h"
#include "motor_controller.h"
#include "dash.h"
#include "vcu_io.h"
#include "cmsis_os2.h"

static osThreadId_t s_can_thread = NULL;

osThreadId_t can_task_get_handle(void) {
    return s_can_thread;
}

void can_task_register_handle(osThreadId_t handle) {
    s_can_thread = handle;
}

void can_task(void *arg) {
    (void)arg;

    uint8_t  rolling_counter   = 0;
    uint8_t  prev_vsm_state    = mc_vsm_state();
    uint32_t prev_fault_bitmap = mc_fault_bitmap();

    // Set green at startup. Other devices on network may set low
    DashLedCmd_t led_cmd = {
        .imd_ok = 1u,
        .bms_ok = 1u,
    };
    dash_set_leds(&led_cmd);

    for (;;) {
        MotorControllerCmd_t cmd;
        motor_controller_get_cmd(&cmd);
        cmd.rolling_counter = rolling_counter++ & 0x0Fu;
        can_tx_send_inverter_cmd(&cmd);
        dash_tx_cmd();

        uint8_t  vsm    = mc_vsm_state();
        uint32_t faults = mc_fault_bitmap();

        if (vsm != prev_vsm_state) {
            LOG_EVENT(LOG_LEVEL_INFO, EVT_STATE_CHANGE, prev_vsm_state, vsm);
            prev_vsm_state = vsm;
        }

        if (faults != prev_fault_bitmap) {
            LogEventId_t evt = (faults != 0u) ? EVT_FAULT_SET : EVT_FAULT_CLEAR;
            LogLevel_t   lvl = (faults != 0u) ? LOG_LEVEL_ERROR : LOG_LEVEL_INFO;
            LOG_EVENT(lvl, evt, prev_fault_bitmap, faults);
            prev_fault_bitmap = faults;
        }

        osDelay(CAN_TASK_PERIOD_MS);
    }
}
