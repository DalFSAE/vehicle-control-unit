// fsm.c
#define LOG_MODULE LOG_SRC_FSM
#include "fsm.h"
#include "vcu_io.h"
#include "log.h"

// ---------------------------------------------------------------------------
// Transition table: [current state][event] -> next state
// ---------------------------------------------------------------------------

static const FsmState_t transition_table[ST_COUNT][FSM_EV_COUNT] = {
    // [current state]    OK           READY        NOTREADY     RTD          RTD_REV      STOP
    [ST_ENTRY] = {ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY},
    [ST_STANDBY] = {ST_STANDBY, ST_NEUTRAL, ST_STANDBY, ST_STANDBY, ST_STANDBY, ST_STANDBY},
    [ST_NEUTRAL] = {ST_NEUTRAL, ST_NEUTRAL, ST_STANDBY, ST_FORWARD, ST_REVERSE, ST_STANDBY},
    [ST_FORWARD] = {ST_FORWARD, ST_FORWARD, ST_STANDBY, ST_FORWARD, ST_FORWARD, ST_NEUTRAL},
    [ST_REVERSE] = {ST_REVERSE, ST_REVERSE, ST_STANDBY, ST_REVERSE, ST_REVERSE, ST_NEUTRAL},
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
        .pedal_plaus = FAULT_RESP_RETURN_NEUTRAL,
        .sensor_range = FAULT_RESP_RETURN_NEUTRAL,
        .can_timeout = FAULT_RESP_RETURN_NEUTRAL,
    };
    return cfg;
}
// Convert fault policy into FSM behavior:
// Only "return to neutral" faults trigger a STOP event,
// all others leave the state unchanged (FSM_EV_OK).
static FsmEvent_t fault_response_to_event(FmsFaultResponse_t resp) {
    return (resp == FAULT_RESP_RETURN_NEUTRAL) ? FSM_EV_STOP : FSM_EV_OK;
}

// ---------------------------------------------------------------------------
// State functions
// ---------------------------------------------------------------------------

static FsmEvent_t entry_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    (void)in;
    out->can_watchdog = true;
    out->tssi_en = true;
    out->relay_always_on = true;
    out->relay_inverter = true;
    return FSM_EV_OK;
}

static FsmEvent_t standby_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    out->throttle_enabled = false;
    out->relay_always_on = true;
    out->relay_inverter = false;
    out->can_watchdog = true;
    out->tssi_en = false;

    if (in->fwrd_switch && in->ts_active) {
        return FSM_EV_READY;
    }

    return FSM_EV_OK;
}

static FsmEvent_t neutral_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    out->throttle_enabled = false;
    out->relay_always_on = true;
    out->relay_inverter = true;
    out->can_watchdog = true;
    out->tssi_en = false;

    if (in->fwrd_switch && in->rtd_button && in->brake_pressed) {
        out->buzzer_beep_ms = 1000;
        out->motor_direction = MOTOR_DIR_FORWARD;
        return FSM_EV_RTD;
    }
    return FSM_EV_OK;
}

static FsmEvent_t forward_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    if (!in->fwrd_switch) {
        out->throttle_enabled = false;
        return FSM_EV_STOP;
    }

    if (fault_active(in, FAULT_APPS_DISAGREE)) {
        out->throttle_enabled = false;
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_APPS_DISAGREE, cfg->apps_disagree);
        return fault_response_to_event(cfg->apps_disagree);
    }
    if (fault_active(in, FAULT_PEDAL_PLAUS)) {
        out->throttle_enabled = false;
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_PEDAL_PLAUS, cfg->pedal_plaus);
        return fault_response_to_event(cfg->pedal_plaus);
    }
    if (fault_active(in, FAULT_SENSOR_RANGE)) {
        out->throttle_enabled = false;
        LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_SENSOR_RANGE, cfg->sensor_range);
        return fault_response_to_event(cfg->sensor_range);
    }

    out->throttle_enabled = true;
    return FSM_EV_OK;
}
static FsmEvent_t reverse_state(const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    (void)cfg;
    (void)in;
    (void)out;
    // TODO
    return FSM_EV_OK;
}

typedef FsmEvent_t (*StateFn_t)(const FsmFaultConfig_t *, const VcuInputs *, VcuOutputs *);

static const StateFn_t state_fns[ST_COUNT] = {
    [ST_ENTRY] = entry_state,     [ST_STANDBY] = standby_state, [ST_NEUTRAL] = neutral_state,
    [ST_FORWARD] = forward_state, [ST_REVERSE] = reverse_state,
};

FsmState_t step_fsm(FsmState_t current, const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out) {
    FsmEvent_t ev = state_fns[current](cfg, in, out);
    return transition_table[current][ev];
}