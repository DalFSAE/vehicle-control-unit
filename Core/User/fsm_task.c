#define LOG_MODULE LOG_SRC_FSM
#define FSM_PERIOD_MS 10u

#include "fsm_task.h"
#include "fsm.h"
#include "vehicle_state.h"
#include "vcu_io.h"
#include "cmsis_os2.h"
#include "log.h"


void fsm_task(void *arg) {
    (void)arg;

    const FsmFaultConfig_t fault_cfg = FaultConfig_default();

    FsmState_t state = ST_ENTRY;
    VcuInputs  in = {0};
    VcuOutputs out = {0};

    for (;;) {
        vcu_read_inputs(&in);
        FsmState_t next = step_fsm(state, &fault_cfg, &in, &out);
        // log state changes
        if (next != state) {
            LOG_EVENT(LOG_LEVEL_INFO, EVT_STATE_CHANGE, state, next);
        }
        state = next;
        vcu_apply_outputs(&out);
        osDelay(FSM_PERIOD_MS);
    }
}
