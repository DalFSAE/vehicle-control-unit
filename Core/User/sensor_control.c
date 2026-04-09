#define LOG_MODULE LOG_SRC_SENSOR
#include "log.h"

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"
#include "main.h"
#include "math.h"
#include "task.h"

#include "sensor_control.h"
#include "pedal_logic.h"
#include "log.h"
#include "dms_defines.h"
#include "vehicle_state.h"
#include "input_control.h"


#define ADC_RESOLUTION_MAX 4096
#define ADC_RESOLUTION_MIN 0
#define ADC_BUFFER_LEN 8 // Should be equal to the number of ADC channels
#define SENSOR_DEBUG_LOG_PERIOD_MS 100U
#define VERBOSE false
#define MOCK true

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
extern DAC_HandleTypeDef hdac;

volatile uint16_t adc_buf[ADC_BUFFER_LEN];

static osThreadId_t s_sensor_thread = NULL;

void sensor_control_register_thread(osThreadId_t thread_id) {
    s_sensor_thread = thread_id;
}



static SensorInfo_t g_sensors[NUM_SENSORS] = {
    [APPS1] = {"APPS1", 1.0f, 2.0f, 0, 0.0f},
    [APPS2] = {"APPS2", 1.0f, 2.0f, 0, 0.0f},
    [FBPS] = {"FBPS", 0.0f, 3.3f, 0, 0.0f},
    [RBPS] = {"RBPS", 0.0f, 3.3f, 0, 0.0f},
};

// updates the gloabl vehicle state with latest inputs 
static void publish_inputs(const SensorInfo_t *sensors, uint32_t fault_flags) {
    g_vcu.brake_pressed =
        sensors[FBPS].normalizedValue > BRAKE_LIGHT_THRESH;
    g_vcu.throttle_request = sensors[APPS1].normalizedValue;
    g_vcu.fault_flags = fault_flags;

    get_driver_inputs(&g_vcu);
}

void set_dac_out(uint32_t dacOut) {
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacOut);
}

// init object
void sensor_init(void) {
    HAL_TIM_Base_Start(&htim2);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, ADC_BUFFER_LEN);
    HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);
    return;
}

void process_adc(SensorInfo_t *sensors) {
    if (sensors == NULL) {
        return;
    }

#if MOCK
    // Known-good test values that stay within the configured sensor ranges.
    sensors[APPS1].normalizedValue = 0.25f;
    sensors[APPS2].normalizedValue = 0.27f;
    sensors[FBPS].normalizedValue  = 0.05f;
    sensors[RBPS].normalizedValue  = 0.05f;

    sensors[APPS1].currentAdcValue = (int)pedal_denormalize(sensors[APPS1].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
    sensors[APPS2].currentAdcValue = (int)pedal_denormalize(sensors[APPS2].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
    sensors[FBPS].currentAdcValue  = (int)pedal_denormalize(sensors[FBPS].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
    sensors[RBPS].currentAdcValue  = (int)pedal_denormalize(sensors[RBPS].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
    return;
#else
    sensors[RBPS].currentAdcValue  = adc_buf[0]; // todo: confirm channel mapping
    sensors[FBPS].currentAdcValue  = adc_buf[1];
    sensors[APPS1].currentAdcValue = adc_buf[2];
    sensors[APPS2].currentAdcValue = adc_buf[3];

    sensors[RBPS].normalizedValue = pedal_adc_to_normalized(
        sensors[RBPS].currentAdcValue, sensors[RBPS].voltageMin, sensors[RBPS].voltageMax, ADC_RESOLUTION_MAX);
    sensors[FBPS].normalizedValue = pedal_adc_to_normalized(
        sensors[FBPS].currentAdcValue, sensors[FBPS].voltageMin, sensors[FBPS].voltageMax, ADC_RESOLUTION_MAX);
    sensors[APPS1].normalizedValue = pedal_adc_to_normalized(
        sensors[APPS1].currentAdcValue, sensors[APPS1].voltageMin, sensors[APPS1].voltageMax, ADC_RESOLUTION_MAX);
    sensors[APPS2].normalizedValue = pedal_adc_to_normalized(
        sensors[APPS2].currentAdcValue, sensors[APPS2].voltageMin, sensors[APPS2].voltageMax, ADC_RESOLUTION_MAX);
#endif
}

void sensorInputTask(void *argument) {
    (void)argument;
    sensor_init();
    uint32_t lastDebugLogMs = 0u;

    pedalStatus_t pedalStatus = {
        .latchStatus = PDP_OKAY,
        .offsetStatus = PDP_OKAY,
        .sensorStatus = PDP_OKAY,
    };

    for (;;) {

        // collect ADC data and update global state
        process_adc(g_sensors);
        publish_inputs(g_sensors, g_vcu.fault_flags);

        // Publish to shared vehicle state; FSM reads these each cycle
        g_vcu.fault_flags = pedal_check_faults(&pedalStatus, g_sensors, NUM_SENSORS);

        // Get driver inptus
        get_driver_inputs(&g_vcu);

        uint32_t nowMs = HAL_GetTick();
        if (VERBOSE && ((nowMs - lastDebugLogMs) >= SENSOR_DEBUG_LOG_PERIOD_MS)) {
            lastDebugLogMs = nowMs;
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, APPS1, (uint32_t)g_sensors[APPS1].currentAdcValue);
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, APPS2, (uint32_t)g_sensors[APPS2].currentAdcValue);
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, FBPS, (uint32_t)g_sensors[FBPS].currentAdcValue);
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, RBPS, (uint32_t)g_sensors[RBPS].currentAdcValue);
        }
        
        // todo: improve how this task handles sleep
        osDelay(1);
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
    (void)hadc;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    (void)hadc;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (s_sensor_thread != NULL) {
        vTaskNotifyGiveFromISR(s_sensor_thread, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
