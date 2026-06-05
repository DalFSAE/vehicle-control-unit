#pragma once
// usb_cmd.h
// Custom command interface for USB_DEVICE\App\usbd_cdc_if.c

#include <stdbool.h>
#include <stdint.h>
#include "fsm_task.h"

typedef enum {
    CMD_SPOOF_SET       = 0x01,
    CMD_SPOOF_CLEAR     = 0x02,
    CMD_REQUEST_OUTPUTS = 0x03,
    CMD_REQUEST_STATE   = 0x04,
    CMD_STEP            = 0x05,
    CMD_FAULT_INJECT    = 0x06,
    CMD_RESET           = 0x07,
    CMD_ECHO            = 0x45,
    CMD_REPLY_OUTPUT    = 0x83,
    CMD_REPLY_STATE     = 0x84,
} UsbCmd_t;

typedef enum {
    FSM_CMD_SPOOF_SET,
    FSM_CMD_SPOOF_CLR,
    FSM_CMD_STEP,
    FSM_CMD_RESET,
    FSM_CMD_FAULT_INJECT,
} FsmCmdType_t;

typedef struct {
    FsmCmdType_t type;
    union {
        VcuInputs spoof;
        uint32_t  fault_flags;
    } payload;
} FsmCmd_t;

uint32_t usb_cmd_rx(const uint8_t *buf, uint32_t len);