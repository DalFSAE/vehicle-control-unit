#pragma once 

#include "stm32f4xx_hal.h"
#include "stdbool.h"

// Enum for relay channels
// todo: update with channel function
typedef enum {
    RELAY_ALWAYS_ON,
    RELAY_BRAKE_LIGHT,
    RELAY_INVERTER,
    RELAY_FANS,
    RELAY_SDC,
} RelayChannel_t;


// Relay Controls

void relay_init(void);
void relay_enable(RelayChannel_t ch);
void relay_disable(RelayChannel_t ch);
void relay_toggle(RelayChannel_t ch);
uint32_t relay_get_state(RelayChannel_t ch);


//