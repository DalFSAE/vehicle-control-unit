#include "torque_processing.h"

static motor_torque_config_t config;
static bool initialized = false; //tracks if the motor configuration was initialized

//this function returns an enum value defined in the header file
static torque_state_t determine_state(float value){
    if(value < config.pedal_lo){
        return TORQUE_STATE_ERROR;
    }
    else if(value <= config.accel_min){
        return TORQUE_STATE_REGEN_FULL;
    }
    else if(value <= config.coast_lo){
        return TORQUE_STATE_REGEN_RAMP;
    }
    else if(value <= config.coast_hi){
        return TORQUE_STATE_COAST;
    }
    else if(value <= config.accel_max){
        return TORQUE_STATE_ACCEL_RAMP;
    }
    else if(value <= config.pedal_hi){
        return TORQUE_STATE_ACCEL_FULL;
    }
}


//right now, error just throws no torque output, which is the same as state_coast. Wasnt sure the way to add a proper error message
static float state_error(void){
    return 0.0f;
}

static float state_regen_full(void){
    return config.regen_torque_limit;
}

static float state_regen_ramp(float value){
    return (1 - (value - config.accel_min) / 
                    (config.coast_lo - config.accel_min)) * 
                    config.regen_torque_limit;
}

static float state_coast(void){
    return 0.0f;
}

static float state_accel_ramp(float value){
    return (value - config.coast_hi) / 
           (config.accel_max - config.coast_hi) * 
           config.motor_torque_limit;
}

static float state_accel_full(void){
    return config.motor_torque_limit;
}

void motor_torque_init(motor_torque_config_t cfg){
    config = cfg;
    initialized = true;
}

float motor_torque(SensorInfo_t sensor){
    torque_state_t state = determine_state(sensor.normalizedValue);
    switch(state){
        case TORQUE_STATE_ERROR:      return state_error();
        case TORQUE_STATE_REGEN_FULL: return state_regen_full();
        case TORQUE_STATE_REGEN_RAMP: return state_regen_ramp(sensor.normalizedValue);
        case TORQUE_STATE_COAST:      return state_coast();
        case TORQUE_STATE_ACCEL_RAMP: return state_accel_ramp(sensor.normalizedValue);
        case TORQUE_STATE_ACCEL_FULL: return state_accel_full();
        default:                      return state_error();
    }
}