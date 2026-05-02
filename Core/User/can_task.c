#define LOG_MODULE LOG_SRC_CAN
#include "log.h"

#include "can_task.h"
#include "motor_controller.h"
#include "vehicle_state.h"
#include "cmsis_os2.h"

static osThreadId_t s_can_thread = NULL;

osThreadId_t can_task_get_handle(void) {
    return s_can_thread;
}

void can_task(void *arg) {
    (void)arg;
    s_can_thread = osThreadGetId();

    uint8_t rolling_counter = 0;

    for (;;) {
        MotorControllerCmd_t cmd;
        motor_controller_get_cmd(&cmd);
        cmd.rolling_counter = rolling_counter++;
        can_tx_send_inverter_cmd(&cmd);

        osDelay(CAN_TASK_PERIOD_MS);
    }
}
