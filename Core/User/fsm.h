#pragma once

// Response taken when a fault is detected in the forward/reverse state.
typedef enum {
    FAULT_RESP_CUT_THROTTLE,   // zero throttle, stay in current state
    FAULT_RESP_RETURN_NEUTRAL, // drop back to neutral
} FaultResponse_t;

typedef struct {
    FaultResponse_t apps_disagree;
    FaultResponse_t pedal_plaus;
    FaultResponse_t sensor_range;
} FaultConfig_t;

void fsm_task(void *arg);
