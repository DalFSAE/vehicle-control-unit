#pragma once

#include "vehicle_state.h"
#include "fsm.h"
#include <stdint.h>

#define FSM_SETTLE_MS (FSM_PERIOD_MS * 4u)

// Shared spoof state 
// modify fields then call vcu_spoof_inputs(&g_spoof).
extern VcuInputs g_spoof;

void suspend_sensor(void);
void resume_sensor(void);

// Zeroes g_spoof and pushes it to the spoof layer.
void clear_inputs(void);

// Drive FSM from STANDBY -> NEUTRAL. Sensor task must already be suspended.
void walk_to_neutral(void);

// Drive FSM from NEUTRAL -> FORWARD.
void walk_to_forward(void);

// Linearly ramp g_spoof.throttle_request from start to end over `steps`
// increments, delaying delta_ms between each step.
void ramp_throttle(float start, float end, uint32_t steps, uint32_t delta_ms);
