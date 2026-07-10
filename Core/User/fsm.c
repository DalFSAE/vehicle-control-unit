// fsm.c
#define LOG_MODULE LOG_SRC_FSM
#include "fsm.h"
#include "vcu_io.h"
#include "log.h"

// ---------------------------------------------------------------------------
// Transition table: [current state][event] -> next state
// ---------------------------------------------------------------------------

static const FsmState_t transition_table[ST_COUNT][FSM_EV_COUNT] = {
    // [state]       OK           READY        NOTREADY     RTD          RTD_REV      STOP        FAULT
    [ST_ENTRY]   = {ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_FAULT},
    [ST_STANDBY] = {ST_STANDBY, ST_NEUTRAL, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_FAULT},
    [ST_NEUTRAL] = {ST_NEUTRAL, ST_NEUTRAL, ST_STANDBY, ST_FORWARD, ST_REVERSE, ST_STANDBY, ST_FAULT},
    [ST_FORWARD] = {ST_FORWARD, ST_FORWARD, ST_STANDBY, ST_FORWARD, ST_FORWARD, ST_NEUTRAL, ST_FAULT},
    [ST_REVERSE] = {ST_REVERSE, ST_REVERSE, ST_STANDBY, ST_REVERSE, ST_REVERSE, ST_NEUTRAL, ST_FAULT},
    [ST_FAULT]   = {ST_FAULT,   ST_FAULT,   ST_FAULT,   ST_FAULT,   ST_FAULT,   ST_FAULT,   ST_FAULT},
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool fault_active(const VcuInputs *in, uint32_t flag) {
    return (in->fault_flags & flag) != 0;
}

FsmFaultConfig_t FaultConfig_default(void) {
    FsmFaultConfig_t cfg = {
        .apps_disagree = FAULT_RESP_CUT_THROTTLE,
        .pedal_plaus   = FAULT_RESP_RETURN_NEUTRAL,
        .sensor_range  = FAULT_RESP_RETURN_NEUTRAL,
        .can_timeout   = FAULT_RESP_RETURN_NEUTRAL,
        .ts_lost       = FAULT_RESP_RETURN_NEUTRAL,
    };
    return cfg;
}

// Apply a fault response policy: cut throttle, set any additional outputs,
// and return the FSM event that drives the transition.
static FsmEvent_t fault_response(FmsFaultResponse_t resp, const VcuInputs *in, VcuOutputs *out) {
    (void)in;
    out->throttle_enabled = false; // every fault cuts throttle
    switch (resp) {
        case FAULT_RESP_CUT_THROTTLE:
            return FSM_EV_OK; // stay in current state, throttle zeroed
        case FAULT_RESP_RETURN_NEUTRAL:
            return FSM_EV_STOP; // drop to neutral
        case FAULT_RESP_SDC_OPEN:
            out->sdc_open = true;
            return FSM_EV_STOP; // open SDC, drop to neutral
        case FAULT_RESP_LATCH_FAULT:
            out->sdc_open = true;
            return FSM_EV_FAULT; // open SDC, latch in ST_FAULT
        default:
            return FSM_EV_OK;
    }
}

static FsmEvent_t check_for_drive_fault(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    if (fault_active(in, FAULT_APPS_DISAGREE)) {
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_APPS_DISAGREE, cfg->apps_disagree);
        return fault_response(cfg->apps_disagree, in, out);
    }
    if (fault_active(in, FAULT_PEDAL_PLAUS)) {
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_PEDAL_PLAUS, cfg->pedal_plaus);
        return fault_response(cfg->pedal_plaus, in, out);
    }
    if (fault_active(in, FAULT_SENSOR_RANGE)) {
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_SENSOR_RANGE, cfg->sensor_range);
        return fault_response(cfg->sensor_range, in, out);
    }
    if (fault_active(in, FAULT_CAN_TIMEOUT)) {
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_CAN_TIMEOUT, cfg->can_timeout);
        return fault_response(cfg->can_timeout, in, out);
    }
    return FSM_EV_OK;
}

// ---------------------------------------------------------------------------
// State functions
// ---------------------------------------------------------------------------

static FsmEvent_t entry_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    (void)in;
    // Output matches the safe default set by outputs_default().
    return FSM_EV_OK;
}

static FsmEvent_t standby_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    out->brake_light = in->brake_pressed;
    out->sdc_open    = false;

    if (in->fwrd_switch && in->ts_active) {
        return FSM_EV_READY;
    }

    return FSM_EV_OK;
}

static FsmEvent_t neutral_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    out->relay_inverter = true;
    out->brake_light    = in->brake_pressed;
    out->sdc_open       = false;

#if VCU_ENABLE_REVERSE
    if (!(in->fwrd_switch || in->rvrs_switch)) {
        return FSM_EV_NOTREADY;
    }
    if (in->rvrs_switch && in->rtd_button && in->brake_pressed) {
        out->buzzer_beep_ms  = 1000;
        out->motor_direction = MOTOR_DIR_REVERSE;
        return FSM_EV_RTD_REV;
    }
#else
    if (!in->fwrd_switch) {
        return FSM_EV_NOTREADY;
    }
#endif

    if (in->fwrd_switch && in->rtd_button && in->brake_pressed) {
        out->buzzer_beep_ms  = 1000;
        out->motor_direction = MOTOR_DIR_FORWARD;
        return FSM_EV_RTD;
    }
    return FSM_EV_OK;
}

static FsmEvent_t forward_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    out->relay_inverter = true;
    out->brake_light    = in->brake_pressed;
    out->sdc_open       = false;

    if (!in->fwrd_switch) {
        // Driver deliberately released switch; soft stop, not a fault.
        return FSM_EV_STOP;
    }
    if (!in->ts_active) {
        // Tractive system may open for many valid reasons, not necessarily a fault
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, 0, cfg->ts_lost);
        return fault_response(cfg->ts_lost, in, out);
    }
    FsmEvent_t drive_fault = check_for_drive_fault(cfg, in, out);
    if (drive_fault != FSM_EV_OK) {
        return drive_fault;
    }

    out->throttle_enabled = true;
    out->throttle_request = in->throttle_request;
    return FSM_EV_OK;
}

static FsmEvent_t reverse_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
#if VCU_ENABLE_REVERSE
    out->relay_inverter  = true;
    out->brake_light     = in->brake_pressed;
    out->motor_direction = MOTOR_DIR_REVERSE;
    out->sdc_open        = false;

    if (!in->rvrs_switch) {
        return FSM_EV_STOP;
    }
    if (!in->ts_active) {
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, 0, cfg->ts_lost);
        return fault_response(cfg->ts_lost, in, out);
    }
    FsmEvent_t drive_fault = check_for_drive_fault(cfg, in, out);
    if (drive_fault != FSM_EV_OK) { 
        return drive_fault;
    } 

    out->throttle_enabled = true;
    out->throttle_request = in->throttle_request * VCU_REVERSE_THROTTLE_SCALE;
    return FSM_EV_OK;
#else
    (void)cfg;
    (void)in;
    (void)out;
    return FSM_EV_OK; // unreachable: neutral_state never emits FSM_EV_RTD_REV when locked out
#endif
}

static FsmEvent_t fault_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    // Latched: SDC open, throttle and inverter off are already the safe
    // default from outputs_default(). Only a power cycle (ST_ENTRY reset)
    // can exit this state.
    out->brake_light = in->brake_pressed;
    return FSM_EV_FAULT;
}

typedef FsmEvent_t (*StateFn_t)(const FsmFaultConfig_t *, const VcuInputs *, VcuOutputs *);

static const StateFn_t state_fns[ST_COUNT] = {
    [ST_ENTRY]   = entry_state,     
    [ST_STANDBY] = standby_state, 
    [ST_NEUTRAL] = neutral_state,
    [ST_FORWARD] = forward_state, 
    [ST_REVERSE] = reverse_state, 
    [ST_FAULT]   = fault_state,
};

FsmState_t step_fsm(FsmState_t current, const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    outputs_default(out);
    FsmEvent_t ev = state_fns[current](cfg, in, out);
    return transition_table[current][ev];
}
