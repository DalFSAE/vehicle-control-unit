#include "motor_controller.h"
#include "sensor_control.h"

#define TORQUE_OUTPUT_DAC_MAX_VALUE 4095u

void init_motor_controller() {

}

void torque_output_update(float request, bool enabled) {
    if (!enabled) {
        set_dac_out(0);
        return;
    }
    set_dac_out((uint32_t)(request * TORQUE_OUTPUT_DAC_MAX_VALUE));
}
