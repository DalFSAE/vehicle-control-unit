#pragma once

#include "vehicle_state.h"

// Populate VcuInputs from hardware and shared sensor state.
void vcu_read_inputs(VcuInputs *in);

// Drive hardware from VcuOutputs.
void vcu_apply_outputs(const VcuOutputs *out);
