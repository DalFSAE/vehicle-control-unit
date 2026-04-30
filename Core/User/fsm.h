#pragma once

#include "vehicle_state.h"

// Response taken when a fault is detected by the vehicle.
typedef enum {
    FAULT_RESP_CUT_THROTTLE,   // zero throttle, stay in current state
    FAULT_RESP_RETURN_NEUTRAL, // drop back to neutral
    FAULT_RESP_SDC_OPEN,       // opens the shutdown circuit (SDC)
    FAULT_RESP_LATCH_FAULT,    // open sdc and latch fault (power cycle required)
} FmsFaultResponse_t;

// Configures how the vehicle responds to a fault scenario
// Add any likely or common faults
typedef struct {
    FmsFaultResponse_t apps_disagree;
    FmsFaultResponse_t pedal_plaus;
    FmsFaultResponse_t sensor_range;
    FmsFaultResponse_t can_timeout;
} FsmFaultConfig_t;

FsmFaultConfig_t FaultConfig_default(void);

// States & events
typedef enum {
    ST_ENTRY,   // VCU init, enable GLV relays
    ST_STANDBY, // driver switch CLOSED - inverter relay off, waiting
    ST_NEUTRAL, // TS_ACTIVE confirmed, ready to accept RTD
    ST_FORWARD, // vehicle is "ready-to-drive" - VCU sending torque commands
    ST_REVERSE, // not legal at FSAE. If used, there must be *strict* power
    ST_COUNT
} FsmState_t;

typedef enum {
    FSM_EV_OK,       // stay in current state
    FSM_EV_READY,    // switch UP + ts_active both true
    FSM_EV_NOTREADY, // switch DOWN or ts_active lost
    FSM_EV_RTD,      // rtd sequence complete - forward
    FSM_EV_RTD_REV,  // rtd sequence complete - reverse
    FSM_EV_STOP,     // soft stop - return to neutral
    FSM_EV_COUNT
} FsmEvent_t;

FsmState_t step_fsm(FsmState_t current, const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out);
