#pragma once

#include "vcu_io.h"

// Compile-time reverse lockout. Reverse is not legal at FSAE competition;
// enable only for private test sessions. Default: off.
#ifndef VCU_ENABLE_REVERSE
#define VCU_ENABLE_REVERSE 0
#endif
// Throttle scale applied in reverse to limit wheel speed.
#define VCU_REVERSE_THROTTLE_SCALE 0.2f

// Response taken when a fault is detected by the vehicle.
typedef enum {
    FAULT_RESP_CUT_THROTTLE,   // zero throttle, stay in current state
    FAULT_RESP_RETURN_NEUTRAL, // drop back to neutral
    FAULT_RESP_SDC_OPEN,       // open the shutdown circuit (SDC), drop to neutral
    FAULT_RESP_LATCH_FAULT,    // open SDC and latch in ST_FAULT (power cycle required)
} FmsFaultResponse_t;

// Configures how the vehicle responds to a fault scenario.
typedef struct {
    FmsFaultResponse_t apps_disagree;
    FmsFaultResponse_t pedal_plaus;
    FmsFaultResponse_t sensor_range;
    FmsFaultResponse_t can_timeout;
    FmsFaultResponse_t ts_lost;
} FsmFaultConfig_t;

FsmFaultConfig_t FaultConfig_default(void);

// States & events
typedef enum {
    ST_ENTRY,   // VCU init, enable GLV relays
    ST_STANDBY, // driver switch CLOSED - inverter relay off, waiting
    ST_NEUTRAL, // TS_ACTIVE confirmed, ready to accept RTD
    ST_FORWARD, // vehicle is "ready-to-drive" - VCU sending torque commands
    ST_REVERSE, // not legal at FSAE; locked out unless VCU_ENABLE_REVERSE=1
    ST_FAULT,   // latched fault state; SDC open, power cycle required to exit
    ST_COUNT
} FsmState_t;

typedef enum {
    FSM_EV_OK,       // stay in current state
    FSM_EV_READY,    // switch UP + ts_active both true
    FSM_EV_NOTREADY, // switch DOWN or ts_active lost
    FSM_EV_RTD,      // rtd sequence complete - forward
    FSM_EV_RTD_REV,  // rtd sequence complete - reverse
    FSM_EV_STOP,     // soft stop - return to neutral
    FSM_EV_FAULT,    // latch into ST_FAULT (SDC open, power cycle required)
    FSM_EV_COUNT
} FsmEvent_t;

FsmState_t step_fsm(FsmState_t current, const FsmFaultConfig_t *cfg, const VcuInputs *in, VcuOutputs *out);
