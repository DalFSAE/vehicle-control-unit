#pragma once

#include "stm32f4xx_hal.h"

typedef enum {
    RELAY_ALWAYS_ON,
    RELAY_BRAKE_LIGHT,
    RELAY_INVERTER,
    RELAY_FANS,
    RELAY_SDC,
    RELAY_AUX,
    RELAY_COUNT     // must be at end
} RelayChannel_t;

void relay_init(void);
void relay_enable(RelayChannel_t ch);
void relay_disable(RelayChannel_t ch);
void relay_toggle(RelayChannel_t ch);
uint32_t relay_get_state(RelayChannel_t ch);
