#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    FAULT_NONE          = 0,
    FAULT_APPS_DISAGREE = (1u << 0),
    FAULT_PEDAL_PLAUS   = (1u << 1),
    FAULT_SENSOR_RANGE  = (1u << 2),
} FaultFlags_t;

typedef enum {
    MOTOR_DIR_FORWARD = 0,
    MOTOR_DIR_REVERSE = 1,
} MotorDir_t;

// Shared state written by the sensor task; read by the world adapter.
typedef struct {
    volatile float    throttle_request; // [0.0, 1.0]
    volatile bool     brake_pressed;
    volatile uint32_t fault_flags;      // FaultFlags_t bitmask
} VehicleState_t;

extern VehicleState_t g_vehicle;

// Runtime IO types
// Inputs consumed by the FSM each cycle (populated by vcu_read_inputs).
typedef struct {
    float    throttle_request; // [0.0, 1.0]
    bool     brake_pressed;
    uint32_t fault_flags;      // FaultFlags_t bitmask
    bool     rtd_pressed;      // DASH_RTD_BUTTON
    bool     dash_switch_on;   // DASH_SWITCH, debounced
} VcuInputs;

// Outputs produced by the FSM each cycle (applied by vcu_apply_outputs).
typedef struct {
    // Relays
    bool relay_always_on;
    bool relay_inverter;
    bool brake_light;

    // Digital outputs
    bool mc_brake_sw;  // pin level: true = not braking (active-low on MC)
    bool can_watchdog;
    bool tssi_en;      // true = TSSI light disabled

    // Motor / torque
    MotorDir_t motor_direction;
    bool       throttle_enabled;
    float      throttle_request; // [0.0, 1.0] passed through from inputs

    // Buzzer: non-zero triggers a beep of that duration; cleared each cycle.
    uint32_t buzzer_beep_ms;
} VcuOutputs;
