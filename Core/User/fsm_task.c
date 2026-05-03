#define LOG_MODULE LOG_SRC_FSM

#include "fsm_task.h"
#include "fsm.h"
#include "vehicle_state.h"
#include "vcu_io.h"
#include "dash.h"
#include "cmsis_os2.h"
#include "log.h"

volatile FsmState_t g_fsm_state = ST_ENTRY;

// Packs the boolean output fields into a bitmask for change detection and logging.
// Keep in sync with the EVT_IO_CHANGE formatter in log.c.
static uint32_t pack_outputs(const VcuOutputs *o) {
    return ((uint32_t)o->relay_always_on  << 0u)
         | ((uint32_t)o->relay_inverter   << 1u)
         | ((uint32_t)o->brake_light      << 2u)
         | ((uint32_t)o->mc_brake_sw      << 3u)
         | ((uint32_t)o->can_watchdog     << 4u)
         | ((uint32_t)o->tssi_en          << 5u)
         | ((uint32_t)o->motor_direction  << 6u)
         | ((uint32_t)o->throttle_enabled << 7u);
}

void fsm_task(void *arg) {
    (void)arg;

    const FsmFaultConfig_t fault_cfg = FaultConfig_default();

    FsmState_t state = ST_ENTRY;
    VcuInputs  in    = {0};
    VcuOutputs out   = {0};
    uint32_t   prev_out_bits = UINT32_MAX; // force log on first cycle

    for (;;) {
        vcu_gather_inputs(&in);
        FsmState_t next = step_fsm(state, &fault_cfg, &in, &out);

        if (next != state) {
            LOG_EVENT(LOG_LEVEL_INFO, EVT_STATE_CHANGE, state, next);
        }

        uint32_t out_bits = pack_outputs(&out);
        if (out_bits != prev_out_bits) {
            LOG_EVENT(LOG_LEVEL_DEBUG, EVT_IO_CHANGE,
                      out_bits,
                      (uint32_t)(out.throttle_request * 1000.0f));
            prev_out_bits = out_bits;
        }

        state = next;
        g_fsm_state = state;

        DashLedCmd_t leds = {
            .imd_ok = 1u,
            .bms_ok = 1u,
        };
        dash_set_leds(&leds);

        vcu_apply_outputs(&out);
        osDelay(FSM_PERIOD_MS);
    }
}
