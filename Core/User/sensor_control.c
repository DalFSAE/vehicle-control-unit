#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h" 
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"
#include "main.h"
#include "math.h"
#include "task.h"

#include "sensor_control.h"
#include "app_main.h"
#include "dms_logging.h"
#include "dms_defines.h"
#include "io_control.h"

#define ADC_RESOLUTION_MAX 4096
#define ADC_RESOLUTION_MIN 0
#define ADC_BUFFER_LEN 8 // Should be equal to the number of ADC channels

#define OFFSET_THRESHOLD 10 // Percent 

#define CUT_MOTOR_SIGNAL 0


extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2; 
extern DAC_HandleTypeDef hdac;

extern osThreadId_t sensor_inputHandle;

volatile uint16_t adc_buf[ADC_BUFFER_LEN];

typedef struct pedalStatus_s {
    PDP_StatusTypeDef offsetStatus;
    PDP_StatusTypeDef latchStatus;
    PDP_StatusTypeDef sensorStatus;
} pedalStatus_t;
 


///@brief  Normalization
///@return normalized value [0.0, 1.0]
float normalize(uint16_t value, uint16_t min, uint16_t max){
    return (float)(value - min) / (max - min); // TODO: Profile performance
}

///@brief  De normalization of value, to be used for ADC output
///@return Inverse of normalized value [min - max]
uint32_t denormalize(float normalizedValue , uint16_t min, uint16_t max){
    return (uint32_t)(normalizedValue * (max - min) + min);
}

float percentDifference(float a, float b) {
    if (a == b) {
        return 0.0f; // If both numbers are equal, percent difference is 0%
    } else {
        float avg = (a + b) / 2.0f;
        float diff = fabs(a - b); // Absolute difference
        return (diff / avg);
    }
}


/// @brief Throttle Input Module
/// @return Throttle value scaled to desired map
float linear_interpolation(float adc_input, float xarray[11], float yarray[11]) {
	float x0 = 0.0f, x1 = 0.0f, y0 = 0.0f, y1 = 0.0f;
	int i = 0;
	while (xarray[i] < adc_input && i < 11) {
		i++;
	}
	x0 = xarray[i - 1];
	x1 = xarray[i];
	y0 = yarray[i - 1];
	y1 = yarray[i];

	float outputValue = (y1 + (adc_input - x1) * ((y1 - y0) / (x1 - x0))); 
	return outputValue;
} 

/// @brief  APPS Agreement Check. Checks if both APPS sensors are within
///         %error threshold of each other.
/// @retval PDP_OKAY no fault
/// @retval PDP_ERROR AAC_fault, difference between pedal sensors > %threshold
PDP_StatusTypeDef apps_offset_check(float apps1, float apps2, float thresh) {
    float diff = percentDifference(apps1, apps2);
    if (diff >= thresh) {
		return PDP_ERROR;
    }
    return PDP_OKAY;
}
/// @brief Check if both the throttle and break are being pressed. If so latch the    
///        throttle until it returns below a value
/// @param pedal Current pedal state 
/// @param apps  Accelerator pedal position sensor value [0.0, 1.0]
/// @param bps   Brake pressure sensor value [0.0, 1.0]
/// @param appsLatchThresh    Accelerator pedal position sensor threshold [0.0, 1.0]
/// @param bpsLatchThresh     Brake pressure sensor threshold [0.0, 1.0]
/// @param appsRestThreshold  Throttle will not unlatch until below this value [0.0, 1.0]
/// @return PDP_StatusTypeDef
PDP_StatusTypeDef pedal_plasability_check(pedalStatus_t *pedal, float apps, float bps, float appsLatchThresh, float bpsLatchThresh, float appsRestThresh) {
    
    if (apps > appsLatchThresh && bps > bpsLatchThresh) {
		return PDP_ERROR;
	} else if (pedal->latchStatus != PDP_OKAY && apps < appsRestThresh){     // Check if latch can be reset
        return PDP_OKAY;                                                        // Disable latch
    } else if (pedal->latchStatus != PDP_OKAY && apps > appsLatchThresh){       // Waiting for latch to reset fault
        return PDP_ERROR;
    } else {
        return pedal->latchStatus;
    }
}

/// @brief Check if a normalized value is out of range
/// @param normalizedValue Value to check, typically [0.0, 1.0]
/// @param minRange Minium threshold to cause a fault [less than 0] 
/// @param maxRange Maximum threshold to cause a fault [greater than 0]
/// @return PDP_StatusTypeDef
PDP_StatusTypeDef sensor_out_of_range(float normalizedValue, float  minRange, float maxRange){
    if (normalizedValue > maxRange || normalizedValue > minRange){
        return PDP_ERROR;
    } 
    return PDP_OKAY;
}


// TODO: Update to use RT for throttle 
/// @param throttlePercent throttle percent [0, 1.0]
void set_throttle(float throttlePercent){
    // uint32_t dacOut = denormalize(throttlePercent, ADC_RESOLUTION_MIN, ADC_RESOLUTION_MAX);
    // HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacOut);  
}

// TODO:  
/// @param dacOut
void set_dac_out(uint32_t dacOut){
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacOut);  
}

void sensor_init() {
    // init object 
    HAL_TIM_Base_Start(&htim2);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, ADC_BUFFER_LEN);
    HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);
    return;
}


/// @brief Store Raw ADC values to sensor struct
/// @param sensors 
void set_sensor_adc_values(SensorInfo_t sensors[]) {
    for (int i = 0; i < NUM_SENSORS; i++){
        sensors[i].currentAdcValue = adc_buf[i];
    }
}

///@brief Convert ADC value to normalized value based on min and max voltage
///@param adcValue Raw ADC value to convert
///@param minVoltage Minimum voltage corresponding to 0 ADC value
///@param maxVoltage Maximum voltage corresponding to maximum ADC value
///@return Normalized value in the range [0.0, 1.0]. Values may be larger then 1
float adc_to_normalized(int adcValue, float minVoltage, float voltageMax, int adcMax) {
    float adcRefVoltage = 3.3f;    
    // Convert ADC value to voltage
    float adc_voltage = (adcValue / (float)adcMax) * adcRefVoltage;

    // Normalize voltage to a range of 0 to 1
    float normalized = (adc_voltage - minVoltage) / (voltageMax - minVoltage);
    return normalized;
}

bool check_faults(pedalStatus_t *pedalStatus, SensorInfo_t *sensors){
    static float appsLatchThresh = 0.4f, // As percent of throttle
                 bpsLatchThresh  = 0.1f, 
                 appsResetThresh = 0.3f;

    pedalStatus->offsetStatus = apps_offset_check(sensors[APPS1].normalizedValue, sensors[APPS2].normalizedValue, 0.2);
    pedalStatus->latchStatus =  pedal_plasability_check(pedalStatus, sensors[APPS1].normalizedValue, sensors[FBPS].normalizedValue, appsLatchThresh, bpsLatchThresh, appsResetThresh);

    float minRange = -0.1f,
          maxRange =  1.1f;      

    for (int i = 0; i < NUM_SENSORS; i++){
        PDP_StatusTypeDef rc = sensor_out_of_range(sensors[i].normalizedValue, minRange, maxRange);
        if (rc == PDP_ERROR){
            pedalStatus->sensorStatus = PDP_ERROR;
            return false;
        }
    }
    
    if (pedalStatus->latchStatus != PDP_OKAY){
        return false;
    }
    else if (pedalStatus->offsetStatus != PDP_OKAY){
        return false;
    }
    return true;
}

bool check_brake_light(float brakeLight){
    // Check brake light 
    if (brakeLight > BRAKE_LIGHT_THRESH){
        relay_enable(RELAY_BRAKE_LIGHT);
        return true;
    }
    // TODO Enable brake light if deceleration rate exceeds 1.3m/s2 (approximately 0.13 g)
    relay_disable(RELAY_BRAKE_LIGHT);

    return false;
}

void process_adc(SensorInfo_t *sensors){
    
    sensors[FBPS].currentAdcValue  = adc_buf[1];
    sensors[RBPS].currentAdcValue  = adc_buf[0];    // todo. fix
    // curr sensor = adc_buf[1]
    sensors[APPS1].currentAdcValue = adc_buf[2];
    sensors[APPS2].currentAdcValue = adc_buf[3];
    
    // sensors[FBPS].currentAdcValue = sensors[APPS2].currentAdcValue; // FOR TESTING so that pedal checks can be done !! 

    for (int i = 0; i < NUM_SENSORS; ++i){
        sensors[i].normalizedValue = adc_to_normalized(sensors[i].currentAdcValue, sensors[i].voltageMin, sensors[i].voltageMax, ADC_RESOLUTION_MAX);
    }
    // Do scaling and linear approximations as necessary 
    return;
 }

void output_throttle(SensorInfo_t *sensors) {

    set_throttle(sensors[APPS1].normalizedValue); 

    static uint32_t count = 0;
    if (count > 10) {

        int apps1 = (int)(sensors[APPS1].normalizedValue * 100);
        int apps2 = (int)(sensors[APPS2].normalizedValue * 100);
        int fbps = (int)(sensors[FBPS].normalizedValue * 100);
        int rbps = (int)(sensors[RBPS].normalizedValue * 100);

        dms_printf( "[SENSOR] APPS1: %d%% \n"
                    "[SENSOR] APPS2: %d%% \n"
                    "[SENSOR] FBPS:  %d%% \n"
                    "[SENSOR] RPBS:  %d%% \n\n\r",
                    apps1, apps2, fbps, rbps);
        count = 0;
    }
    count++;
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
    .latchStatus = PDP_OKAY,
    .offsetStatus = PDP_OKAY,
    .sensorStatus = PDP_OKAY,
    };

    dms_printf("[DEBUG] Sensor input task started\n\r");
    for(;;) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Block task until ADC is ready

         HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);    // LEDs are used for time profiling       
        // ADC Processing 
        process_adc(sensors);
        bool outputThrottle =  check_faults(&pedalStatus, sensors);
        
        // Check if brake light should be enabled
        if (true){          
            check_brake_light(sensors[FBPS].normalizedValue); 
        }
        
        // throttle Output
        // todo move to function
        outputThrottle = true; // DEBUG !!
        if (outputThrottle == true){
            output_throttle(sensors);
  
        }
        
        // Cleanup         
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
        // osDelay(10);
    }
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
	(void)hadc;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    (void)hadc;
    // Unblock sensorInputTask(...)  
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(sensor_inputHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    // HAL_GPIO_TogglePin(BREAK_LIGHT_PORT, BRAKE_LIGHT_PIN);
}


void enable_throttle(bool enable) {
    return;
}