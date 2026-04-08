#pragma once

#include <stdbool.h>

// Update torque output. Routes to DAC now; swap implementation for CAN later.
void torque_output_update(float request, bool enabled);
