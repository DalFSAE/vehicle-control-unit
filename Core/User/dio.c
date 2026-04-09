#include "dio.h"

#include "main.h"
#include <stdbool.h>

// Digital IO Pins
static inline void dio_map(DIO_Channel_t ch,
    GPIO_TypeDef **port, uint16_t *pin) {
    switch (ch) {
        case PCB_USER_BUTTON:   *port = GPIOA; *pin = GPIO_PIN_0; break;
        case CAN_WATCHDOG:      *port = GPIOB; *pin = GPIO_PIN_7; break; 
        case TSSI_EN:           *port = GPIOC; *pin = GPIO_PIN_10; break;
        case DASH_RTD_BUTTON:   *port = GPIOD; *pin = GPIO_PIN_0; break;
        case DIO_D1:            *port = GPIOD; *pin = GPIO_PIN_1; break;
        case BMS_STATUS:        *port = GPIOD; *pin = GPIO_PIN_2; break;
        case MC_FORWARD_SW:     *port = GPIOD; *pin = GPIO_PIN_4; break;
        case MC_REGEN_SW:       *port = GPIOD; *pin = GPIO_PIN_5; break;
        case MC_BRAKE_SW:       *port = GPIOD; *pin = GPIO_PIN_6; break;
        case DASH_SWITCH:       *port = GPIOD; *pin = GPIO_PIN_7; break;
        case BUZZER:            *port = GPIOE; *pin = GPIO_PIN_13; break;
        default:                *port = NULL;  *pin = 0; break;
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
