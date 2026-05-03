#pragma once

#include "vehicle_state.h"

// Assemble VcuInputs from all sensor and device module getters.
void vcu_gather_inputs(VcuInputs *in);

// Drive hardware from VcuOutputs; sends motor controller command to CAN layer.
void vcu_apply_outputs(const VcuOutputs *out);

// HIL test spoof interface: when active, vcu_gather_inputs() returns the
// injected struct verbatim instead of reading hardware.
void vcu_spoof_inputs(const VcuInputs *spoof);
void vcu_clear_spoof(void);
