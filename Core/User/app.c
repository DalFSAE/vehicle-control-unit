#include "app.h"

#include "cmsis_os2.h"
#include "main.h"
//new includes
#include "adc.h" 
#include "tim.h"
#include <stdio.h>

enum {
    APP_HEARTBEAT_PERIOD_MS = 250U,
    APP_HEARTBEAT_STACK_SIZE = 256U * 4U,

    //10 Hz telemetry rate
    APP_TELEMETRY_PERIOD_MS = 100U,
    APP_TELEMETRY_STACK_SIZE = 512U * 4U
};

//external handles
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

//buffer that holds raw ADC reading (P1 and P2)
static uint32_t adc_raw_buffer[2] = {0};

static void app_heartbeat_task(void *argument);
static void app_telemetry_task(void *argument);

static osThreadId_t app_heartbeat_task_handle;
static osThreadId_t app_telemetry_task_handle;

static const osThreadAttr_t app_heartbeat_task_attributes = {
    .name = "app_heartbeat",
    .stack_size = APP_HEARTBEAT_STACK_SIZE,
    .priority = (osPriority_t)osPriorityNormal,
};

static const osThreadAttr_t app_telemetry_task_attributes = {
    .name = "app_telemetry",
    .stack_size = APP_TELEMETRY_STACK_SIZE,
    .priority = (osPriority_t)osPriorityNormal,
};


void app_init(void)
{
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_RESET);

    // Start ADC1 DMA continuous scanning into adc_raw_buffer
    HAL_ADC_Start_DMA(&hadc1, adc_raw_buffer, 2);

    // Start TIM2 counter for flow meter pulse reading
    HAL_TIM_Base_Start(&htim2);    
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

    app_telemetry_task_handle = osThreadNew(
        app_telemetry_task,
        NULL,
        &app_telemetry_task_attributes);

    if (app_telemetry_task_handle == NULL) {
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

static void app_telemetry_task(void *argument)
{
    (void)argument;

    for (;;) {
        // 1. Capture system timestamp in ms
        uint32_t timestamp = osKernelGetTickCount();

        // 2. Read Pressure Sensors from DMA buffer (0-4095 counts to 0-3.3V)
        float v1 = (adc_raw_buffer[0] * 3.3f) / 4095.0f;
        float v2 = (adc_raw_buffer[1] * 3.3f) / 4095.0f;
        
        // Scaled conversion to PSI (adjust formula for your sensor calibration)
        float pressure1_psi = (v1 - 0.5f) * 37.5f; 
        float pressure2_psi = (v2 - 0.5f) * 37.5f;

        // 3. Read Flow Meter pulses from hardware timer
        uint32_t pulses = __HAL_TIM_GET_COUNTER(&htim2);
        __HAL_TIM_SET_COUNTER(&htim2, 0); // Reset counter for next window
        float flow_rate_lpm = (float)pulses / 7.5f; // Calibration factor

        // 4. Output timestamped data over UART via printf
        printf("[%lu ms] P1: %.2f PSI | P2: %.2f PSI | FLOW: %.2f LPM\r\n",
               timestamp,
               pressure1_psi,
               pressure2_psi,
               flow_rate_lpm);

        osDelay(APP_TELEMETRY_PERIOD_MS);
    }
}