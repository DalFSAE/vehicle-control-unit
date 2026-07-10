#pragma once

#include "fsm.h"
#include "node.h"
#include <stdint.h>

#define CAN_ID_DASH_CMD    0x201U   // VCU -> Dash: LED command
#define CAN_ID_DASH_STATUS 0x202U   // Dash -> VCU: (reserved)

// TX frame layout (CAN_ID_DASH_CMD, 3 bytes):
//   Byte 0: reserved
//   Byte 1: imd_ok - IMD status LED
//   Byte 2: bms_ok - BMS status LED
typedef struct {
    uint8_t imd_ok;
    uint8_t bms_ok;
} DashLedCmd_t;

// Must be called after OS starts (creates mutex).
void dash_init(void);

// Called from fsm_task each cycle to update the cached LED state.
void dash_set_leds(const DashLedCmd_t *cmd);

// Called from can_task each cycle to transmit the dash command frame.
// Returns true if the CAN mailbox accepted the frame.
bool dash_tx_cmd(void);

// CAN RX handler, called from ISR via can_bus dispatch.
void dash_rx(uint32_t id, const uint8_t *data, size_t len);
extern const CanNode_t dash_node;
