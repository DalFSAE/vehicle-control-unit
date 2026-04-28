#pragma once

#include "vehicle_state.h"

// true: spoof adc data
#define MOCK_IO false

// Populate VcuInputs from hardware and shared sensor state.
void vcu_read_inputs(VcuInputs *in);

// Drive hardware from VcuOutputs.
void vcu_apply_outputs(const VcuOutputs *out);
