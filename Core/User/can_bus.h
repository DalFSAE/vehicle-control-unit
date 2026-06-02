#pragma once

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Start CAN, configure accept-all filter, activate FIFO0 RX notification.
// mode: CAN_MODE_NORMAL for production, CAN_MODE_LOOPBACK for self-test.
void can_bus_init(CAN_HandleTypeDef *hcan, uint32_t mode);

// Transmit a standard (non-extended) CAN frame. Returns true on success.
bool can_bus_transmit(uint32_t id, const uint8_t *data, uint8_t len);

// Pass NULL to disable. Used by loopback tests to capture frames.
typedef void (*CanRxHook_t)(uint32_t id, const uint8_t *data, size_t len);
void can_bus_set_rx_hook(CanRxHook_t hook);
