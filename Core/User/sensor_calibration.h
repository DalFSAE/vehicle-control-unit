#pragma once

#include "sensor_types.h"

typedef enum{
    CALIBRATING_IDLE,
    CALIBRATING_MIN,
    CALIBRATING_MAX,
} CalibrationStateMachine_t;

void cal_init(void);
void cal_start(SensorType_t channel); //function to begin sensor calibration
void cal_mark_min(void);//function to mark minimum voltage
void cal_mark_max(void); //function to mark maximum voltage

void cal_commit(void); //function to commit calibration 
void cal_cancel(void); //function to cancel calibration

float cal_get_min(SensorType_t channel); //return minimum voltage
float cal_get_max(SensorType_t channel); //return maximum voltage
