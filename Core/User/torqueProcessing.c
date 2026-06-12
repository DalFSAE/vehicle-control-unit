#include "torqueProcessing.h"

static motor_torque_config_t config;
static bool initialized = false; //tracks if the motor configuration was initialized

void motor_torque_init(motor_torque_config_t cfg){
    config = cfg;
    initialized = true;
}

float motor_torque(float apps_percent){

    if(!initialized) return 0.0f; //will return 0.0 if motor_torque_init(cfg) was not called

    float torque_output = 0; //this will only ever stay 0 if apps_percent > pedal_hi, which matches the graph, maybe not what we want
   
    //should this function include clamping logic? i.e if apps_percent < 0 => apps_percent = 0

    if(apps_percent < config.pedal_lo){
        torque_output = 0.0f;
    }
    else if(apps_percent <= config.accel_min){
        torque_output = config.regen_torque_limit;
    }
    else if (apps_percent < config.coast_lo) {
    torque_output = (1 - (apps_percent - config.accel_min) / 
                    (config.coast_lo - config.accel_min)) * 
                    config.regen_torque_limit;
    }
    else if(apps_percent <= config.coast_hi){
        torque_output = 0.0f;
    }
   else if (apps_percent < config.accel_max) {
    torque_output = (apps_percent - config.coast_hi) / 
                    (config.accel_max - config.coast_hi) * 
                    config.motor_torque_limit;
    }
    else if(apps_percent <= config.pedal_hi){
        torque_output = config.motor_torque_limit;
    }
    return torque_output;
}