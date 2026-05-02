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

// Lifecycle — must be called after OS starts (creates mutex).
void motor_controller_init(void);

// Thread-safe command cache — written by FSM via vcu_apply_outputs().
void motor_controller_set_cmd(const MotorControllerCmd_t *cmd);
void motor_controller_get_cmd(MotorControllerCmd_t *out);

// Transmit M192 command frame. rolling_counter must be set by caller (can_task).
void can_tx_send_inverter_cmd(const MotorControllerCmd_t *cmd);

// CAN node RX handler — called from ISR via can_bus dispatch.
void inverter_rx(uint32_t id, const uint8_t *data, size_t len);
extern const CanNode_t inverter_node;

// State getters consumed by vcu_gather_inputs() and can_task.
bool     mc_is_ready(void);     // VSM state >= MC_VSM_READY
bool     mc_has_timeout(void);  // time since last M170 > MC_HEARTBEAT_TIMEOUT_MS
uint32_t mc_fault_bitmap(void); // post_fault | run_fault, for telemetry
