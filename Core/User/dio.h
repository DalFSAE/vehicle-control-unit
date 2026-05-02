#pragma once

#include <stdbool.h>


// Digital Pins 
typedef enum {
    DASH_RTD_BUTTON = 0, 
    PCB_USER_BUTTON,
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
