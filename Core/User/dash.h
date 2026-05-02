#pragma once

#include "fsm.h"
#include "node.h"
#include <stdint.h>

#define CAN_ID_DASH_CMD    0x700U   // VCU -> Dash: LED command
#define CAN_ID_DASH_STATUS 0x701U   // Dash -> VCU: (reserved)

#define DASH_LED_TEST_MS 2000U

// TX frame layout (CAN_ID_DASH_CMD, 5 bytes):
//   Byte 0: imd_ok   - IMD status LED
//   Byte 1: bms_ok   - BMS status LED
//   Byte 2: rtd      - ready-to-drive LED
//   Byte 3: fault    - general fault LED
//   Byte 4: led_test - all LEDs on (managed by dash.c for DASH_LED_TEST_MS after boot)
typedef struct {
    uint8_t imd_ok;
    uint8_t bms_ok;
    uint8_t rtd;
    uint8_t fault;
} DashLedCmd_t;

// Must be called after OS starts (creates mutex).
void dash_init(void);

// Called from fsm_task each cycle to update the cached LED state.
void dash_set_leds(const DashLedCmd_t *cmd);

// Called from can_task each cycle to transmit the dash command frame.
void dash_tx_cmd(void);

// CAN RX handler, called from ISR via can_bus dispatch.
void dash_rx(uint32_t id, const uint8_t *data, size_t len);
extern const CanNode_t dash_node;
