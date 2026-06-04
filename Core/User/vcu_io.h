//vcu_io.h
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    FAULT_NONE          = 0,
    FAULT_APPS_DISAGREE = (1u << 0),
    FAULT_PEDAL_PLAUS   = (1u << 1),
    FAULT_SENSOR_RANGE  = (1u << 2),
    FAULT_CAN_TIMEOUT   = (1u << 3),
} FaultFlags_t;

typedef enum {
    MOTOR_DIR_FORWARD = 0,
    MOTOR_DIR_REVERSE = 1,
} MotorDir_t;

// Inputs consumed by the FSM each cycle, assembled by vcu_gather_inputs().
// Packed so it can be spoofed wholesale over USB for HIL testing.
typedef struct __attribute__((packed)) {
    uint32_t fault_flags;
    float    throttle_request; // [0.0, 1.0]
    bool     brake_pressed;
    bool     rtd_button;
    bool     fwrd_switch;
    bool     rvrs_switch;
    bool     ts_active;
} VcuInputs;
_Static_assert(sizeof(VcuInputs) == 13, "VcuInputs size mismatch");

// Outputs produced by the FSM each cycle, applied by vcu_apply_outputs().
typedef struct {
    bool       relay_always_on;
    bool       relay_inverter;
    bool       brake_light;
    bool       mc_brake_sw;  // active-low on MC; true = not braking
    bool       can_watchdog;
    bool       tssi_en;      // true = TSSI light disabled
    MotorDir_t motor_direction;
    bool       throttle_enabled;
    float      throttle_request; // [0.0, 1.0]
    uint32_t   buzzer_beep_ms;
} VcuOutputs;

// Assemble VcuInputs from all sensor and device module getters.
void vcu_gather_inputs(VcuInputs *in);

// Drive hardware from VcuOutputs; sends motor controller command to CAN layer.
void vcu_apply_outputs(const VcuOutputs *out);

// HIL test spoof interface: when active, vcu_gather_inputs() returns the
// injected struct verbatim instead of reading hardware.
void vcu_spoof_inputs(const VcuInputs *spoof);
void vcu_clear_spoof(void);
