#define LOG_MODULE LOG_SRC_FSM
#include "fsm.h"
#include "vehicle_state.h"
#include "vcu_io.h"
#include "cmsis_os2.h"
#include "log.h"

#define FSM_PERIOD_MS 10u

// States & events

typedef enum {
    ST_ENTRY,
    ST_NEUTRAL,
    ST_FORWARD,
    ST_REVERSE,
    ST_COUNT
} FsmState_t;

typedef enum {
    FSM_EV_OK,   // stay in current state
    FSM_EV_RTD,  // ready-to-drive
    FSM_EV_STOP, // return to neutral
    FSM_EV_COUNT
} FsmEvent_t;

// Transition table: [current state][event] -> next state
static const FsmState_t transition_table[ST_COUNT][FSM_EV_COUNT] = {
    [ST_ENTRY]   = {[FSM_EV_OK] = ST_NEUTRAL, [FSM_EV_RTD] = ST_NEUTRAL, [FSM_EV_STOP] = ST_NEUTRAL},
    [ST_NEUTRAL] = {[FSM_EV_OK] = ST_NEUTRAL, [FSM_EV_RTD] = ST_FORWARD, [FSM_EV_STOP] = ST_NEUTRAL},
    [ST_FORWARD] = {[FSM_EV_OK] = ST_FORWARD, [FSM_EV_RTD] = ST_FORWARD, [FSM_EV_STOP] = ST_NEUTRAL},
    [ST_REVERSE] = {[FSM_EV_OK] = ST_REVERSE, [FSM_EV_RTD] = ST_REVERSE, [FSM_EV_STOP] = ST_NEUTRAL},
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------


static FsmEvent_t check_fault(const VcuInputs *in, VcuOutputs *out,
                               uint32_t flag, FaultResponse_t resp)
{
    if (!(in->fault_flags & flag)) return FSM_EV_OK;
    out->throttle_enabled = false;
    return (resp == FAULT_RESP_RETURN_NEUTRAL) ? FSM_EV_STOP : FSM_EV_OK;
}

// ---------------------------------------------------------------------------
// State functions
// ---------------------------------------------------------------------------


static FsmEvent_t entry_state(const FaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out)
{
    (void)cfg; 
    (void)in;
    out->can_watchdog    = true;
    out->tssi_en         = true;
    out->relay_always_on = true;
    out->relay_inverter  = true;
    return FSM_EV_OK;
}

static FsmEvent_t neutral_state(const FaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out)
{
    (void)cfg;
    out->throttle_enabled = false;

    if (in->fwrd_switch && in->rtd_button && in->brake_pressed) {
        out->buzzer_beep_ms  = 1000;
        out->motor_direction = MOTOR_DIR_FORWARD;
        return FSM_EV_RTD;
    }
    return FSM_EV_OK;
}

static FsmEvent_t forward_state(const FaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out)
{
    if (!in->fwrd_switch) {
        out->throttle_enabled = false;
        return FSM_EV_STOP;
    }

    FsmEvent_t ev;
    ev = check_fault(in, out, FAULT_APPS_DISAGREE, cfg->apps_disagree); if (ev != FSM_EV_OK) return ev;
    ev = check_fault(in, out, FAULT_PEDAL_PLAUS,   cfg->pedal_plaus);   if (ev != FSM_EV_OK) return ev;
    ev = check_fault(in, out, FAULT_SENSOR_RANGE,  cfg->sensor_range);  if (ev != FSM_EV_OK) return ev;

    out->throttle_enabled = true;
    return FSM_EV_OK;
}

static FsmEvent_t reverse_state(const FaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out)
{
    (void)cfg; (void)in; (void)out;
    // TODO
    return FSM_EV_OK;
}

typedef FsmEvent_t (*StateFn_t)(const FaultConfig_t *, const VcuInputs *, VcuOutputs *);

static const StateFn_t state_fns[ST_COUNT] = {
    [ST_ENTRY]   = entry_state,
    [ST_NEUTRAL] = neutral_state,
    [ST_FORWARD] = forward_state,
    [ST_REVERSE] = reverse_state,
};

// ---------------------------------------------------------------------------
// Task
// ---------------------------------------------------------------------------

void fsm_task(void *arg)
{
    (void)arg;

    const FaultConfig_t fault_cfg = {
        .apps_disagree = FAULT_RESP_CUT_THROTTLE,
        .pedal_plaus   = FAULT_RESP_CUT_THROTTLE,
        .sensor_range  = FAULT_RESP_RETURN_NEUTRAL,
    };

    FsmState_t state = ST_ENTRY;
    VcuInputs  in    = {0};
    VcuOutputs out   = {0};

    for (;;) {
        // todo: create buzzer api
        out.buzzer_beep_ms = 0; // cleared each tick; state sets if needed

        // read from global state object
        vcu_read_inputs(&in);

        // Unconditional output mappings derived directly from inputs
        // todo: find a better spot for these?
        out.brake_light      = in.brake_pressed;
        out.mc_brake_sw      = !in.brake_pressed;
        out.throttle_request = in.throttle_request;

        // finite state machine
        FsmEvent_t  ev       = state_fns[state](&fault_cfg, &in, &out);
        FsmState_t  next     = transition_table[state][ev];
        if (next != state) {
            LOG_EVENT(LOG_LEVEL_INFO, EVT_STATE_CHANGE, state, next);
        }
        state = next;

        // update global state object
        vcu_apply_outputs(&out);

        osDelay(FSM_PERIOD_MS);
    }
}
