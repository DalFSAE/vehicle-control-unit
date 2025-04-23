#pragma once 

#include "stm32f4xx_hal.h"
#include "stdbool.h"

// Enum for relay channels
// todo: update with channel function

#define SWITCH_ON_LEVEL     0          // active-low
#define SWITCH_OFF_LEVEL    1
#define DEBOUNCE_SAMPLES    4          // 4 × 10 ms = 40 ms
#define OFF_HOLDOFF_MS      200        // must stay OFF this long

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
    BMS_STATUS,          // true = bms fault, false = BMS Latched    
    TSSI_EN,             // false = Flash TSSI
    MC_FORWARD_SW,
    MC_REGEN_SW, 
    MC_BRAKE_SW, 
    DASH_SWITCH,
    BUZZER, 
    CAN_WATCHDOG
} DIO_Channel_t;

// helpers 
void dio_write (DIO_Channel_t ch, bool level);   // high / low  
void dio_toggle(DIO_Channel_t ch);               // flip output 
bool dio_read  (DIO_Channel_t ch);               // true = high 
void dio_init(void);

static bool read_dash_switch_filtered(void);