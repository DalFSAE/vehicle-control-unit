#pragma once

#include "fsm.h"
#include "vcu_io.h"

#define FSM_PERIOD_MS 10u

extern volatile FsmState_t g_fsm_state;
extern volatile bool g_fsm_step_requested;
extern volatile bool g_fsm_reset_requested;

FsmState_t fsm_get_state(void);
const VcuOutputs *fsm_get_last_outputs(void);

void fsm_task(void *arg);
