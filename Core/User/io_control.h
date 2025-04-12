#pragma once 

#include "stm32f4xx_hal.h"
#include "stdbool.h"

// Enum for relay channels
// todo: update with channel function
typedef enum {
    RELAY_EN_0,
    RELAY_EN_1,
    RELAY_EN_2,
    RELAY_EN_3,
    RELAY_EN_4,
} RelayChannel_t;


// Relay Controls

void relay_init(void);
void relay_enable(RelayChannel_t ch);
void relay_disable(RelayChannel_t ch);
void relay_toggle(RelayChannel_t ch);
uint32_t relay_get_state(RelayChannel_t ch);
