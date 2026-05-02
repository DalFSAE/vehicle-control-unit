#pragma once

#include "fsm.h"

#define FSM_PERIOD_MS 10u

// Finite State Machine
// state written each cycle by fsm_task, readable by tests.
extern volatile FsmState_t g_fsm_state;

void fsm_task(void *arg);
