#include "app.h"
#include "cmsis_os2.h"
#include "main.h"
#include "board_outputs.h"
#include "output_control.h"
#include "dio.h"

// Tasks
#include "fsm_task.h"
#include "sensor_control.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

#define LOG_MODULE LOG_SRC_APP
#include "log.h"
#include "hardware_test_runner.h"

enum {
    APP_HEARTBEAT_PERIOD_MS = 250U,
    APP_HEARTBEAT_LOG_PERIOD_MS = 1000U,
    APP_HEARTBEAT_STACK_SIZE = 256U * 4U,
    APP_SENSOR_STACK_SIZE = 512U * 4U,
    APP_FSM_STACK_SIZE = 512U * 4U,
};

static void app_heartbeat_task(void *argument);

static BootResult_t s_pre_boot_result;

static osThreadId_t app_heartbeat_task_handle;

static const osThreadAttr_t app_heartbeat_task_attributes = {
    .name = "heartbeat",
    .stack_size = APP_HEARTBEAT_STACK_SIZE,
    .priority = (osPriority_t)osPriorityLow,
};

static const osThreadAttr_t sensor_task_attributes = {
    .name = "sensor_input",
    .stack_size = APP_SENSOR_STACK_SIZE,
    .priority = (osPriority_t)osPriorityAboveNormal,
};

static const osThreadAttr_t fsm_task_attributes = {
    .name = "fsm",
    .stack_size = APP_FSM_STACK_SIZE,
    .priority = (osPriority_t)osPriorityNormal,
};

void app_error_handler(BootStatus_t status) {
    LOG_EVENT(LOG_LEVEL_ERROR, EVT_BOOT, (uint32_t)status, 0u);
    Error_Handler();
}

uint32_t app_init(void) {
    if (!log_init()) {
        app_error_handler(BOOT_ERR_PRE_BOOT_TESTS);
    }
    board_outputs_init();
    dio_init();
    buzzer_init();

    s_pre_boot_result = hardware_test_pre_boot();
    if (s_pre_boot_result.failures != 0u) {
        app_error_handler(BOOT_ERR_PRE_BOOT_TESTS);
    }
    return s_pre_boot_result.failures;
}

void app_post_boot(void) {
    osDelay(500u);
    LOG_EVENT(LOG_LEVEL_INFO, EVT_BOOT, s_pre_boot_result.tests_run, s_pre_boot_result.failures);

    BootResult_t result = hardware_test_post_boot();
    LOG_EVENT(LOG_LEVEL_INFO, EVT_BOOT, result.tests_run, result.failures);
    if (result.failures != 0u) {
        app_error_handler(BOOT_ERR_POST_BOOT_TESTS);
    }
}

void app_create_tasks(void) {
    app_heartbeat_task_handle =
        osThreadNew(app_heartbeat_task, NULL, &app_heartbeat_task_attributes);
    if (app_heartbeat_task_handle == NULL)
        app_error_handler(BOOT_ERR_TASK_CREATE);
    LOG_EVENT(LOG_LEVEL_INFO, EVT_TASK_CREATED, LOG_SRC_APP, 0u);

    osThreadId_t sensor_handle =
        osThreadNew(sensorInputTask, NULL, &sensor_task_attributes);
    if (sensor_handle == NULL)
        app_error_handler(BOOT_ERR_TASK_CREATE);
    sensor_control_register_thread(sensor_handle);
    LOG_EVENT(LOG_LEVEL_INFO, EVT_TASK_CREATED, LOG_SRC_SENSOR, 0u);

    osThreadId_t fsm_handle = osThreadNew(fsm_task, NULL, &fsm_task_attributes);
    if (fsm_handle == NULL)
        app_error_handler(BOOT_ERR_TASK_CREATE);
    LOG_EVENT(LOG_LEVEL_INFO, EVT_TASK_CREATED, 0, 0);
}

static void app_heartbeat_task(void *argument) {
    (void)argument;
    uint32_t elapsed_ms = 0u;

    for (;;) {
        board_output_toggle(OUTPUT_DEBUG_LED3);
        elapsed_ms += APP_HEARTBEAT_PERIOD_MS;
        if (elapsed_ms >= APP_HEARTBEAT_LOG_PERIOD_MS) {
            LOG_EVENT(LOG_LEVEL_INFO, EVT_HEARTBEAT, elapsed_ms, 0u);
            elapsed_ms = 0u;
        }
        osDelay(APP_HEARTBEAT_PERIOD_MS);
    }
}
