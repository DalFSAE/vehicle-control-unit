#include "io_control.h"

// lib
#include "stdbool.h"
#include "main.h"

// Digital IO Pins

static inline void dio_map(DIO_Channel_t ch,
    GPIO_TypeDef **port, uint16_t *pin) {
    switch (ch) {
        case DASH_RTD_BUTTON: *port = GPIOD; *pin = GPIO_PIN_0; break;   // button
        case DIO_D1: *port = GPIOD; *pin = GPIO_PIN_1; break;
        case BMS_STATUS: *port = GPIOD; *pin = GPIO_PIN_2; break;
        case TSSI_EN: *port = GPIOC; *pin = GPIO_PIN_10; break;
        case MC_FORWARD_SW: *port = GPIOD; *pin = GPIO_PIN_4; break;
        case MC_REGEN_SW: *port = GPIOD; *pin = GPIO_PIN_5; break;
        case MC_BRAKE_SW: *port = GPIOD; *pin = GPIO_PIN_6; break;
        case DASH_SWITCH: *port = GPIOD; *pin = GPIO_PIN_7; break;
        case BUZZER: *port = GPIOE; *pin = GPIO_PIN_13; break;
        case CAN_WATCHDOG: *port = GPIOB; *pin = GPIO_PIN_7; break; // DO NOT USE UNTIL PCB IS FIXED
        default:     *port = NULL;  *pin = 0;          break;
    }
}

void dio_init(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);    // enable IO
    dio_write(DIO_D1, true);    // send 5V to dash switches
    dio_write(MC_FORWARD_SW, true); // true == 0 on the MC
    dio_write(MC_REGEN_SW, true); // true == 0 on the MC
    dio_write(MC_BRAKE_SW, true); // true == 0 on the MC

    
}

void dio_write(DIO_Channel_t ch, bool level) {
    GPIO_TypeDef *port; uint16_t pin;
    dio_map(ch, &port, &pin);
    if (port) HAL_GPIO_WritePin(port, pin,
                                level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void dio_toggle(DIO_Channel_t ch) {
    GPIO_TypeDef *port; uint16_t pin;
    dio_map(ch, &port, &pin);
    if (port) HAL_GPIO_TogglePin(port, pin);
}

bool dio_read(DIO_Channel_t ch) {
    GPIO_TypeDef *port; uint16_t pin;
    dio_map(ch, &port, &pin);
    return port ? (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) : false;
}


/*****************************************************************
*  Simple 1-bit software debounce + asymmetric OFF delay
*****************************************************************/


static bool     sw_stable     = false;
static uint8_t  sw_cnt        = 0;
static uint32_t off_start_ms  = 0;

bool read_dash_switch_filtered(void)
{
   bool raw = dio_read(DASH_SWITCH);          // true = HIGH (OFF), false = LOW (ON)

   // Debounce
   if (raw == sw_stable)                      // still the same level
   {
       sw_cnt = 0;                            // reset counter
   }
   else if (++sw_cnt >= DEBOUNCE_SAMPLES)     // changed & stayed for N samples
   {
       sw_stable = raw;
       sw_cnt    = 0;
       if (sw_stable == SWITCH_OFF_LEVEL)     // just went OFF -> start hold-off timer
           off_start_ms = HAL_GetTick();
   }

   // OFF hysteresis
   if (sw_stable == SWITCH_OFF_LEVEL)
   {
       if (HAL_GetTick() - off_start_ms < OFF_HOLDOFF_MS)
           return SWITCH_ON_LEVEL;            // still within grace period -> report ON
   }

   return sw_stable;
}


// Motor direction

void mc_set_direction(MotorDir_t dir) {
#if MC_FORWARD_POLARITY_INVERTED
    bool want_forward = (dir == MOTOR_DIR_REVERSE);
#else
    bool want_forward = (dir == MOTOR_DIR_FORWARD);
#endif
    // MC forward switch is active-low: false = forward
    dio_write(MC_FORWARD_SW, !want_forward);
}

// Buzzer Controls

static uint32_t  _beep_start;
static uint32_t  _beep_duration;
static bool      _beep_active;

void buzzer_init(void) {
    dio_write(BUZZER, false);
    _beep_active = false;
}

void buzzer_beep(uint32_t duration_ms) {
    _beep_start    = HAL_GetTick();
    _beep_duration = duration_ms;
    _beep_active   = true;
    dio_write(BUZZER, true);
}

void buzzer_update(void) {
    if (_beep_active &&
        (HAL_GetTick() - _beep_start >= _beep_duration))
    {
        dio_write(BUZZER, false);
        _beep_active = false;
    }
}
