#pragma once

#include "node.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Maximum torque — confirm against motor datasheet and FSAE rules.
#define MC_TORQUE_MAX_NM   150.0f
#define MC_TORQUE_LIMIT_NM 150.0f

// Rinehart VSM state at which the inverter is considered ready to drive.
#define MC_VSM_READY 5u

// M170 must arrive within this window or FAULT_CAN_TIMEOUT is asserted.
#define MC_HEARTBEAT_TIMEOUT_MS 200u

typedef struct {
    bool     inv_enable;
    bool     inv_discharge;
    bool     speed_mode_enable;
    bool     motor_direction_forward;
    float    torque_command_nm;
    float    torque_limit_nm;
    uint16_t speed_command_rpm;
    uint8_t  rolling_counter; // incremented by can_task each TX
} MotorControllerCmd_t;

// Must be called after OS starts (creates mutex).
void motor_controller_init(void);

// Thread-safe command cache - written by FSM via vcu_apply_outputs().
void motor_controller_set_cmd(const MotorControllerCmd_t *cmd);
void motor_controller_get_cmd(MotorControllerCmd_t *out);

// Transmit M192 command frame. rolling_counter must be set by caller (can_task).
void can_tx_send_inverter_cmd(const MotorControllerCmd_t *cmd);

// CAN node RX handler. Called from ISR via can_bus dispatch.
void inverter_rx(uint32_t id, const uint8_t *data, size_t len);
extern const CanNode_t inverter_node;

// State getters consumed by vcu_gather_inputs() and can_task.
bool     mc_is_ready(void);     // VSM state >= MC_VSM_READY
bool     mc_has_timeout(void);  // time since last M170 > MC_HEARTBEAT_TIMEOUT_MS
uint32_t mc_fault_bitmap(void); // post_fault | run_fault, for telemetry
uint8_t  mc_vsm_state(void);    // raw VSM state, for change-detection logging in can_task

// struct containing configurables that will impact the torque algorithm calculation
// these variables were selected via the torque parameters graph on page 53 of 
// https://github.com/DalFSAE/motor-controller-fw/blob/main/docs/Software%20User%20Manual%20(V3_6)%20(1).pdf
// regenerative torque is assumed to be negative.
typedef struct{
    float pedal_lo; //deadzone, ignore anything below this value, e.g. 0.05 -> ignore below 5%
    float pedal_hi; //deadzone, ignore anything above this value, e.g. 0.95 -> ignore above 95%
    float accel_min; //regen approaches 0 from here
    float accel_max; //torque output = max torque here
    float coast_lo; //regen = 0 here
    float coast_hi; //torque output > 0 from here
    float motor_torque_limit; // physical limit of torque output
    float regen_torque_limit; //maximum torque regen
} motor_torque_config_t;

//this enum represents all the possible torque output states
typedef enum {
    TORQUE_STATE_ERROR,
    TORQUE_STATE_REGEN_FULL,
    TORQUE_STATE_REGEN_RAMP,
    TORQUE_STATE_COAST,
    TORQUE_STATE_ACCEL_RAMP,
    TORQUE_STATE_ACCEL_FULL,
} torque_state_t;

void motor_torque_init(motor_torque_config_t cfg);
float motor_torque(float pedal_pos);