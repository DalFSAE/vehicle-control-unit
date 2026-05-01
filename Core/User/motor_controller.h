#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool     inv_enable;
    bool     inv_discharge;
    bool     speed_mode_enable;
    bool     motor_direction_forward;
    float    torque_command_nm;
    float    torque_limit_nm;
    uint16_t speed_command_rpm;
} MotorControllerCmd_t;

void init_motor_controller(void);

void set_cmd_message(const MotorControllerCmd_t *cmd);

// Update torque output. Routes to DAC now; swap implementation for CAN later.
void torque_output_update(float request, bool enabled);

// Build CAN dataframe
void can_tx_send_inverter_cmd(const MotorControllerCmd_t *cmd);