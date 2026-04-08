#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h" 
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"
#include "main.h"
#include "math.h"
#include "task.h"

#include "sensor_control.h"
#include "pedal_logic.h"
#include "vehicle_state.h"
#include "log.h"
#include "dms_defines.h"

#define ADC_RESOLUTION_MAX 4096
#define ADC_RESOLUTION_MIN 0
#define ADC_BUFFER_LEN 8 // Should be equal to the number of ADC channels


extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2; 
extern DAC_HandleTypeDef hdac;

volatile uint16_t adc_buf[ADC_BUFFER_LEN];

 
static osThreadId_t s_sensor_thread = NULL;

void sensor_control_register_thread(osThreadId_t thread_id)
{
    s_sensor_thread = thread_id;
}



void set_dac_out(uint32_t dacOut){
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacOut);  
}

// init object 
void sensor_init(void) {
    HAL_TIM_Base_Start(&htim2);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, ADC_BUFFER_LEN);
    HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);
    return;
}




void process_adc(SensorInfo_t *sensors){
    
    sensors[FBPS].currentAdcValue  = adc_buf[1];
    sensors[RBPS].currentAdcValue  = adc_buf[0];    // todo. fix
    // curr sensor = adc_buf[1]
    sensors[APPS1].currentAdcValue = adc_buf[2];
    sensors[APPS2].currentAdcValue = adc_buf[3];
    
    // sensors[FBPS].currentAdcValue = sensors[APPS2].currentAdcValue; // FOR TESTING so that pedal checks can be done !! 

    for (int i = 0; i < NUM_SENSORS; ++i){
        sensors[i].normalizedValue = pedal_adc_to_normalized(sensors[i].currentAdcValue, sensors[i].voltageMin, sensors[i].voltageMax, ADC_RESOLUTION_MAX);
    }
    // Do scaling and linear approximations as necessary 
    return;
 }


void sensorInputTask(void *argument) {
    (void)argument;
    sensor_init();

    SensorInfo_t sensors[] = {
        [APPS1] = {"APPS1", 1.0f, 2.0f, 0, 0.0f},
        [APPS2] = {"APPS2", 1.0f, 2.0f, 0, 0.0f},
        [FBPS]  = {"FBPS" , 0.0f, 3.3f, 0, 0.0f},
        [RBPS]  = {"RBPS" , 0.0f, 3.3f, 0, 0.0f},       
    };

    pedalStatus_t pedalStatus = {
        .latchStatus  = PDP_OKAY,
        .offsetStatus = PDP_OKAY,
        .sensorStatus = PDP_OKAY,
    };

    // todo: remove and replace with proper logging
    // log_printf("[DEBUG] Sensor input task started\n\r");
    for(;;) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until ADC DMA complete

        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET); // timing probe

        process_adc(sensors);
        g_vehicle.brake_pressed = sensors[FBPS].normalizedValue > BRAKE_LIGHT_THRESH;

        // Publish to shared vehicle state; FSM reads these each cycle
        g_vehicle.throttle_request = sensors[APPS1].normalizedValue;
        g_vehicle.fault_flags = pedal_check_faults(&pedalStatus, sensors, NUM_SENSORS);

        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET); // timing probe
        // osDelay(10);
    }
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
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
