    #include "app.h"

    #include "cmsis_os2.h"
    #include "main.h"
    #include "board_outputs.h"
    #include "io_control.h"
    #include "sensor_control.h"
    #include "fsm.h"

    enum {
        APP_HEARTBEAT_PERIOD_MS  = 250U,
        APP_HEARTBEAT_STACK_SIZE = 256U * 4U,
        APP_SENSOR_STACK_SIZE    = 512U * 4U,
        APP_FSM_STACK_SIZE       = 512U * 4U,
    };

    static void app_heartbeat_task(void *argument);

    static osThreadId_t app_heartbeat_task_handle;

    static const osThreadAttr_t app_heartbeat_task_attributes = {
        .name       = "heartbeat",
        .stack_size = APP_HEARTBEAT_STACK_SIZE,
        .priority   = (osPriority_t)osPriorityLow,
    };

    static const osThreadAttr_t sensor_task_attributes = {
        .name       = "sensor_input",
        .stack_size = APP_SENSOR_STACK_SIZE,
        .priority   = (osPriority_t)osPriorityAboveNormal,
    };

    static const osThreadAttr_t fsm_task_attributes = {
        .name       = "fsm",
        .stack_size = APP_FSM_STACK_SIZE,
        .priority   = (osPriority_t)osPriorityNormal,
    };

    void app_init(void)
    {
        board_outputs_init();
        dio_init();
        buzzer_init();
    }

    void app_create_tasks(void)
    {
        app_heartbeat_task_handle = osThreadNew(app_heartbeat_task, NULL, &app_heartbeat_task_attributes);
        if (app_heartbeat_task_handle == NULL) Error_Handler();

        osThreadId_t sensor_handle = osThreadNew(sensorInputTask, NULL, &sensor_task_attributes);
        if (sensor_handle == NULL) Error_Handler();
        sensor_control_register_thread(sensor_handle);

        osThreadId_t fsm_handle = osThreadNew(fsm_task, NULL, &fsm_task_attributes);
        if (fsm_handle == NULL) Error_Handler();
    }

    static void app_heartbeat_task(void *argument)
    {
        (void)argument;

        for (;;) {
            board_output_toggle(OUTPUT_DEBUG_LED3);
            osDelay(APP_HEARTBEAT_PERIOD_MS);
        }
    }
