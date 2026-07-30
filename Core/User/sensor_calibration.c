#include "sensor_calibration.h"
#include "pedal_logic.h"
#include "sensor_control.h"
#include "sensor_types.h"

#define ADC_RESOLUTION_MAX                                                                                             \
    4096 // this is already defined in sensor_control.c, should i move that definition to sensor_control.h
         // so it works here too?

// both of these arrays are indexed to match the SensorType_t enum in sensor_types.h
static float s_cal_min[NUM_SENSORS];
static float s_cal_max[NUM_SENSORS];
// s_pending variables are used to store calibration values before they are ready to be saved to the calibration arrays
static float s_pending_min                   = 0.0f;
static float s_pending_max                   = 0.0f;
static CalibrationStateMachine_t s_cal_state = CALIBRATING_IDLE;
static SensorType_t s_cal_channel            = APPS1;

void cal_init(void) { // called once at boot
    for (int i = 0; i < NUM_SENSORS; i++) {
        s_cal_min[i] = sensor_get_voltage_min(i);
        s_cal_max[i] = sensor_get_voltage_max(i);
    }
    // s_cal_state = CALIBRATING_IDLE by default
}

void cal_start(SensorType_t channel) { // called each time calibration begins
    if (s_cal_state != CALIBRATING_IDLE) {
        cal_cancel(); // reset variables, but don't return, should it? In this form, cal_start can act like a more
                      // efficient cal_cancel() if we want to restart the process
    }
    s_pending_min = s_pending_max = 0.0f;
    s_cal_channel                 = channel;
    s_cal_state                   = CALIBRATING_MIN;
}
// cal mark min and max use 3.3 as the adc reference voltage, matching pedal_adc_to_normalized() in pedal_logic.c
void cal_mark_min(void) {
    if (s_cal_state != CALIBRATING_MIN) {
        cal_cancel();
        return;
    }
    s_pending_min = (sensor_get_raw(s_cal_channel) / (float)ADC_RESOLUTION_MAX) * 3.3f;
    s_cal_state   = CALIBRATING_MAX;
}
void cal_mark_max(void) {
    if (s_cal_state != CALIBRATING_MAX) {
        cal_cancel();
        return;
    }
    s_pending_max = (sensor_get_raw(s_cal_channel) / (float)ADC_RESOLUTION_MAX) * 3.3f;
}
void cal_commit(void) {
    if (s_cal_state != CALIBRATING_MAX || s_pending_max <= s_pending_min) {
        cal_cancel();
        return;
    }
    s_cal_min[s_cal_channel] = s_pending_min;
    s_cal_max[s_cal_channel] = s_pending_max;
    s_cal_state              = CALIBRATING_IDLE;
}
void cal_cancel(void) {
    s_cal_state   = CALIBRATING_IDLE;
    s_pending_max = s_pending_min = 0.0f;
}
float cal_get_min(SensorType_t channel) {
    return s_cal_min[channel];
}
float cal_get_max(SensorType_t channel) {
    return s_cal_max[channel];
}
