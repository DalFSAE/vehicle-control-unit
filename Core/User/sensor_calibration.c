#include "sensor_calibration.h"
#include "pedal_logic.h"
#include "sensor_control.h"
#include "sensor_types.h"

#define ADC_RESOLUTION_MAX 4096

static float s_cal_min[NUM_SENSORS];
static float s_cal_max[NUM_SENSORS];
static float s_pending_min                   = 0.0f;
static float s_pending_max                   = 0.0f;
static CalibrationStateMachine_t s_cal_state = CALIBRATING_IDLE;
static SensorType_t s_cal_channel            = APPS1;

void cal_init(void) { // called once at boot
    for (int i = 0; i < NUM_SENSORS; i++) {
        s_cal_min[i] = sensor_get_voltage_min(i);
        s_cal_max[i] = sensor_get_voltage_max(i);
    }
}

void cal_start(SensorType_t channel) { // called each time calibration begins
    s_pending_min = s_pending_max = 0.0f;
    s_cal_channel                 = channel;
    s_cal_state                   = CALIBRATING_MIN;
}
void cal_mark_min(void) {
    s_pending_min = pedal_adc_to_normalized(sensor_get_raw(s_cal_channel), sensor_get_voltage_min(s_cal_channel),
                                            sensor_get_voltage_max(s_cal_channel), ADC_RESOLUTION_MAX);
    s_cal_state = CALIBRATING_MAX;
}
void cal_mark_max(void) {
    s_pending_max = s_pending_min = pedal_adc_to_normalized(sensor_get_raw(s_cal_channel), sensor_get_voltage_min(s_cal_channel),
                                            sensor_get_voltage_max(s_cal_channel), ADC_RESOLUTION_MAX);
}
void cal_commit(void) {
    if (s_cal_state != CALIBRATING_MAX) {
        cal_cancel();
        return;
    }
    s_cal_min[s_cal_channel] = s_pending_min;
    s_cal_max[s_cal_channel] = s_pending_max;
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
