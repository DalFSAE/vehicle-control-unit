#include "relay.h"

#include "dms_defines.h"
#include "main.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} RelayGPIO_t;

static const RelayGPIO_t relay_map[RELAY_COUNT] = {
    [RELAY_ALWAYS_ON] = {PWR_ALWAYSON_GPIO_Port, PWR_ALWAYSON_Pin},
    [RELAY_BRAKE_LIGHT] = {PWR_BL_GPIO_Port, PWR_BL_Pin},
    [RELAY_INVERTER] = {PWR_INV_GPIO_Port, PWR_INV_Pin},
    [RELAY_FANS] = {PWR_FANS_GPIO_Port, PWR_FANS_Pin},
    [RELAY_SDC] = {SDC_RELAY_GPIO_Port, SDC_RELAY_Pin},
    [RELAY_AUX] = {PWR_AUX_GPIO_Port, PWR_AUX_Pin},
};

void relay_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    for (int i = 0; i < RELAY_COUNT; i++) {
        GPIO_InitStruct.Pin = relay_map[i].pin;
        HAL_GPIO_Init(relay_map[i].port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(relay_map[i].port, relay_map[i].pin, GPIO_PIN_RESET);
    }
}

void relay_enable(RelayChannel_t ch) {
    if (ch < RELAY_COUNT) {
        HAL_GPIO_WritePin(relay_map[ch].port, relay_map[ch].pin, GPIO_PIN_SET);
    }
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

    return 0xFFFFFFFFu;
}
