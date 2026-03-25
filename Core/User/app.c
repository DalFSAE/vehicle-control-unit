#include "app.h"

#include "cmsis_os2.h"
#include "main.h"
#include "relay.h"

enum {
    APP_HEARTBEAT_PERIOD_MS = 250U,
    APP_HEARTBEAT_STACK_SIZE = 256U * 4U,
};

static void app_heartbeat_task(void *argument);

static osThreadId_t app_heartbeat_task_handle;

static const osThreadAttr_t app_heartbeat_task_attributes = {
    .name = "app_heartbeat",
    .stack_size = APP_HEARTBEAT_STACK_SIZE,
    .priority = (osPriority_t)osPriorityNormal,
};

void app_init(void)
{
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_RESET);

    relay_init();
}

void app_create_tasks(void)
{
    app_heartbeat_task_handle = osThreadNew(
        app_heartbeat_task,
        NULL,
        &app_heartbeat_task_attributes);

    if (app_heartbeat_task_handle == NULL) {
        Error_Handler();
    }
}

static void app_heartbeat_task(void *argument)
{
    (void)argument;

    for (;;) {
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        osDelay(APP_HEARTBEAT_PERIOD_MS);
    }
}
