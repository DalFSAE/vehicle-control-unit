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
#include "vcu_io.h"

#define ADC_RESOLUTION_MAX 4096
#define ADC_BUFFER_LEN 8
#define BRAKE_LIGHT_THRESHOLD 0.15f
#define SENSOR_DEBUG_LOG_PERIOD_MS 100U
#define VERBOSE false

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
extern DAC_HandleTypeDef hdac;

volatile uint16_t adc_buf[ADC_BUFFER_LEN];

static osThreadId_t s_sensor_thread = NULL;

// Private sensor state read via getters
static volatile float    s_throttle = 0.0f;
static volatile bool     s_brake = false;
static volatile uint32_t s_fault_flags = 0u;

static SensorInfo_t g_sensors[NUM_SENSORS] = {
    [APPS1] = {"APPS1", 1.0f, 2.0f, 0, 0.0f},
    [APPS2] = {"APPS2", 1.0f, 2.0f, 0, 0.0f},
    [FBPS] = {"FBPS", 0.0f, 3.3f, 0, 0.0f},
    [RBPS] = {"RBPS", 0.0f, 3.3f, 0, 0.0f},
};

void sensor_control_register_thread(osThreadId_t thread_id) {
    s_sensor_thread = thread_id;
}

osThreadId_t sensor_task_get_handle(void) {
    return s_sensor_thread;
}

float sensor_get_throttle(void) {
    return s_throttle;
}
bool sensor_get_brake(void) {
    return s_brake;
}
uint32_t sensor_get_fault_flags(void) {
    return s_fault_flags;
}

static void publish_inputs(const SensorInfo_t *sensors, uint32_t fault_flags) {
    s_brake = sensors[FBPS].normalizedValue > BRAKE_LIGHT_THRESHOLD;
    s_throttle = sensors[APPS1].normalizedValue;
    s_fault_flags = fault_flags;
}

void set_dac_out(uint32_t dacOut) {
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacOut);
}

void sensor_init(void) {
    HAL_TIM_Base_Start(&htim2);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, ADC_BUFFER_LEN);
    HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);
}

void process_adc(SensorInfo_t *sensors) {
    if (sensors == NULL)
        return;

#if MOCK_ADC
    sensors[APPS1].normalizedValue = 0.25f;
    sensors[APPS2].normalizedValue = 0.27f;
    sensors[FBPS].normalizedValue = 0.05f;
    sensors[RBPS].normalizedValue = 0.05f;

    sensors[APPS1].currentAdcValue = (int)pedal_denormalize(sensors[APPS1].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
    sensors[APPS2].currentAdcValue = (int)pedal_denormalize(sensors[APPS2].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
    sensors[FBPS].currentAdcValue = (int)pedal_denormalize(sensors[FBPS].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
    sensors[RBPS].currentAdcValue = (int)pedal_denormalize(sensors[RBPS].normalizedValue, 0, ADC_RESOLUTION_MAX - 1);
#else
    sensors[RBPS].currentAdcValue = adc_buf[0]; // todo: confirm channel mapping
    sensors[FBPS].currentAdcValue = adc_buf[1];
    sensors[APPS1].currentAdcValue = adc_buf[2];
    sensors[APPS2].currentAdcValue = adc_buf[3];

    sensors[RBPS].normalizedValue = pedal_adc_to_normalized(sensors[RBPS].currentAdcValue, sensors[RBPS].voltageMin,
                                                            sensors[RBPS].voltageMax, ADC_RESOLUTION_MAX);
    sensors[FBPS].normalizedValue = pedal_adc_to_normalized(sensors[FBPS].currentAdcValue, sensors[FBPS].voltageMin,
                                                            sensors[FBPS].voltageMax, ADC_RESOLUTION_MAX);
    sensors[APPS1].normalizedValue = pedal_adc_to_normalized(sensors[APPS1].currentAdcValue, sensors[APPS1].voltageMin,
                                                             sensors[APPS1].voltageMax, ADC_RESOLUTION_MAX);
    sensors[APPS2].normalizedValue = pedal_adc_to_normalized(sensors[APPS2].currentAdcValue, sensors[APPS2].voltageMin,
                                                             sensors[APPS2].voltageMax, ADC_RESOLUTION_MAX);
#endif
}


// Main sensor processing loop. Reads ADC, updates global state, and logs debug info.
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
        process_adc(g_sensors);
        uint32_t faults = pedal_check_faults(&pedalStatus, g_sensors, NUM_SENSORS);
        publish_inputs(g_sensors, faults);

        uint32_t nowMs = HAL_GetTick();
        if (VERBOSE && ((nowMs - lastDebugLogMs) >= SENSOR_DEBUG_LOG_PERIOD_MS)) {
            lastDebugLogMs = nowMs;
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, APPS1, (uint32_t)g_sensors[APPS1].currentAdcValue);
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, APPS2, (uint32_t)g_sensors[APPS2].currentAdcValue);
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, FBPS, (uint32_t)g_sensors[FBPS].currentAdcValue);
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_SENSOR_DEBUG, RBPS, (uint32_t)g_sensors[RBPS].currentAdcValue);
        }

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
