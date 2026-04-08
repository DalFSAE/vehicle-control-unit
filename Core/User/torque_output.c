#include "torque_output.h"
#include "sensor_control.h"
#include "dms_defines.h"

void torque_output_update(float request, bool enabled) {
    if (!enabled) {
        set_dac_out(0);
        return;
    }
    set_dac_out((uint32_t)(request * DAC_MAX_VALUE));
}
