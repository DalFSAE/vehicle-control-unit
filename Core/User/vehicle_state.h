#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    FAULT_NONE = 0,
    FAULT_APPS_DISAGREE = (1u << 0),
    FAULT_PEDAL_PLAUS = (1u << 1),
    FAULT_SENSOR_RANGE = (1u << 2),
} FaultFlags_t;

typedef enum {
    MOTOR_DIR_FORWARD = 0,
    MOTOR_DIR_REVERSE = 1,
} MotorDir_t;

// Shared state written by the sensor task;
typedef struct {
    volatile uint32_t fault_flags;      // FaultFlags_t bitmask
    volatile float    throttle_request; // [0.0, 1.0]
    volatile bool     brake_pressed;
    volatile bool     rtd_button;
    volatile bool     fwrd_switch;
} VcuState_t;

extern VcuState_t g_vcu;

// Runtime IO types
// Inputs consumed by the FSM each cycle
// packed allows spoofing of data via USB interface
typedef struct __attribute__((packed)) {
    // FaultFlags_t bitmask
    uint32_t fault_flags;

    // processed sensor data
    float throttle_request; // [0.0, 1.0]
    bool  brake_pressed;

    // digital inputs
    bool rtd_button;
    bool fwrd_switch;
    bool rvrs_switch;

    // data from the CANbus
    bool ts_active;
} VcuInputs;
_Static_assert(sizeof(VcuInputs) == 13, "VcuInputs size mismatch"); // update as needed

// Outputs produced by the FSM each cycle (applied by vcu_apply_outputs).
typedef struct {
    // Relays
    bool relay_always_on;
    bool relay_inverter;
    bool brake_light;

    // Digital outputs
    bool mc_brake_sw; // pin level: true = not braking (active-low on MC)
    bool can_watchdog;
    bool tssi_en; // true = TSSI light disabled

    // Motor / torque
    MotorDir_t motor_direction;
    bool       throttle_enabled;
    float      throttle_request; // [0.0, 1.0] passed through from inputs

    // Buzzer: non-zero triggers a beep of that duration; cleared each cycle.
    uint32_t buzzer_beep_ms;
} VcuOutputs;

// Data from the CAN bus, used by the FSM
typedef struct {
    volatile bool ts_active;
} CanState_t;

extern CanState_t g_can;