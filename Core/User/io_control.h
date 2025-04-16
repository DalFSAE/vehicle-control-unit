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

// Digital Pins 

typedef enum {
    DASH_RTD_BUTTON = 0, 
    DIO_D1, 
    DIO_D2, 
    DIO_D3,
    DIO_D4,
    DIO_D5, 
    DIO_D6, 
    DASH_FWRD_SW
} DIO_Channel_t;

// helpers 
void dio_write (DIO_Channel_t ch, bool level);   // high / low  
void dio_toggle(DIO_Channel_t ch);               // flip output 
bool dio_read  (DIO_Channel_t ch);               // true = high 
void dio_init(void);