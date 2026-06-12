#include <stdbool.h>
//struct containing configurables that will impact the torque algorithm calculation
//these variables were selected via the torque parameters graph on page 53 of https://github.com/DalFSAE/motor-controller-fw/blob/main/docs/Software%20User%20Manual%20(V3_6)%20(1).pdf
//regenerative torque is assumed to be negative.
typedef struct{
    float peddle_lo; //deadzone, ignore anything below this value, e.g. 0.05 -> ignore below 5%
    float peddle_hi; //deadzone, ignore anything above this value, e.g. 0.95 -> ignore above 95%
    float accel_min; //regen approaches 0 from here
    float accel_max; //torque output = max torque here
    float coast_lo; //regen = 0 here
    float coast_hi; //torque output > 0 from here
    float motor_torque_limit; // physical limit of torque output
    float regen_torque_limit; //maximum torque regen
} motor_torque_config_t;

void motor_torque_init(motor_torque_config_t cfg); //initialize configurables

float motor_torque(float apps_percent); //calculates the motor torque output base on accelerator pedal position sensor(s)