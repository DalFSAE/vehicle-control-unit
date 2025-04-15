#include "io_control.h"
#include "dms_defines.h"

// lib
#include "stdbool.h"
#include "main.h"
// Relay Controls

// GPIO Pin Map
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} RelayGPIO_t;


static const RelayGPIO_t relay_map[RELAY_COUNT] = {
    [RELAY_ALWAYS_ON] = {VBAT_EN_SW0_GPIO_Port, VBAT_EN_SW0_Pin},
    [RELAY_BRAKE_LIGHT] = {VBAT_EN_SW1_GPIO_Port, VBAT_EN_SW1_Pin},
    [RELAY_INVERTER] = {VBAT_EN_SW2_GPIO_Port, VBAT_EN_SW2_Pin},
    [RELAY_FANS] = {VBAT_EN_SW3_GPIO_Port, VBAT_EN_SW3_Pin},
    [RELAY_SDC] = {VBAT_EN_SW4_GPIO_Port, VBAT_EN_SW4_Pin},
};

void relay_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    for (int i = 0; i < RELAY_COUNT; i++) {
        GPIO_InitStruct.Pin = relay_map[i].pin;
        HAL_GPIO_Init(relay_map[i].port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(relay_map[i].port, relay_map[i].pin, GPIO_PIN_RESET); // default off
    }
}

void relay_enable(RelayChannel_t ch) {
    if (ch < RELAY_COUNT)
        HAL_GPIO_WritePin(relay_map[ch].port, relay_map[ch].pin, GPIO_PIN_SET);
}

void relay_disable(RelayChannel_t ch) {
    if (ch < RELAY_COUNT) {
        HAL_GPIO_WritePin(relay_map[ch].port, relay_map[ch].pin, GPIO_PIN_RESET);
    }
}

void relay_toggle(RelayChannel_t ch) {
    if (ch < RELAY_COUNT) {
        HAL_GPIO_TogglePin(relay_map[ch].port, relay_map[ch].pin);
    }
}

uint32_t relay_get_state(RelayChannel_t ch) {
    if (ch < RELAY_COUNT) {
        return HAL_GPIO_ReadPin(relay_map[ch].port, relay_map[ch].pin);
    }
    return 0xFFFFFFFF;
}


// Digital IO Pins

static inline void dio_map(DIO_Channel_t ch,
    GPIO_TypeDef **port, uint16_t *pin) {
    switch (ch) {
        case DIO_D0: *port = GPIOD; *pin = GPIO_PIN_0; break;
        case DIO_D1: *port = GPIOD; *pin = GPIO_PIN_1; break;
        case DIO_D2: *port = GPIOD; *pin = GPIO_PIN_2; break;
        case DIO_D3: *port = GPIOD; *pin = GPIO_PIN_3; break;
        case DIO_D4: *port = GPIOD; *pin = GPIO_PIN_4; break;
        case DIO_D5: *port = GPIOD; *pin = GPIO_PIN_5; break;
        case DIO_D6: *port = GPIOD; *pin = GPIO_PIN_6; break;
        case DIO_D7: *port = GPIOD; *pin = GPIO_PIN_7; break;
        default:     *port = NULL;  *pin = 0;          break;
    }
}

void dio_init(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);\
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