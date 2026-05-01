#include "board_outputs.h"

#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} BoardOutputsGPIO_t;

static const BoardOutputsGPIO_t board_outputs_map[OUTPUT_COUNT] = {
    [OUTPUT_DEBUG_LED4] = {LD4_GPIO_Port, LD4_Pin},
    [OUTPUT_DEBUG_LED3] = {LD3_GPIO_Port, LD3_Pin},
    [OUTPUT_DEBUG_LED5] = {LD5_GPIO_Port, LD5_Pin},
    [OUTPUT_DEBUG_LED6] = {LD6_GPIO_Port, LD6_Pin},
    [OUTPUT_ALWAYS_ON] = {PWR_ALWAYSON_GPIO_Port, PWR_ALWAYSON_Pin},
    [OUTPUT_BRAKE_LIGHT] = {PWR_BL_GPIO_Port, PWR_BL_Pin},
    [OUTPUT_INVERTER] = {PWR_INV_GPIO_Port, PWR_INV_Pin},
    [OUTPUT_FANS] = {PWR_FANS_GPIO_Port, PWR_FANS_Pin},
    [OUTPUT_SDC] = {SDC_RELAY_GPIO_Port, SDC_RELAY_Pin},
    [OUTPUT_AUX] = {PWR_AUX_GPIO_Port, PWR_AUX_Pin},
};

static bool s_output_state[OUTPUT_COUNT];

void board_outputs_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        GPIO_InitStruct.Pin = board_outputs_map[i].pin;
        HAL_GPIO_Init(board_outputs_map[i].port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(board_outputs_map[i].port, board_outputs_map[i].pin, GPIO_PIN_RESET);
        s_output_state[i] = false;
    }
}

void board_output_set(OutputChannel_t ch, bool value) {
    if (ch < OUTPUT_COUNT) {
        HAL_GPIO_WritePin(board_outputs_map[ch].port, board_outputs_map[ch].pin,
                          value ? GPIO_PIN_SET : GPIO_PIN_RESET);
        s_output_state[ch] = value;
    }
}

void board_output_set(OutputChannel_t ch, bool value) {
    if (ch < OUTPUT_COUNT) {
        HAL_GPIO_WritePin(board_outputs_map[ch].port, board_outputs_map[ch].pin,
                          value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void board_output_enable(OutputChannel_t ch) {
    if (ch < OUTPUT_COUNT) {
        HAL_GPIO_WritePin(board_outputs_map[ch].port, board_outputs_map[ch].pin, GPIO_PIN_SET);
        s_output_state[ch] = true;
    }
}

void board_output_disable(OutputChannel_t ch) {
    if (ch < OUTPUT_COUNT) {
        HAL_GPIO_WritePin(board_outputs_map[ch].port, board_outputs_map[ch].pin, GPIO_PIN_RESET);
        s_output_state[ch] = false;
    }
}

void board_output_toggle(OutputChannel_t ch) {
    if (ch < OUTPUT_COUNT) {
        s_output_state[ch] = !s_output_state[ch];
        HAL_GPIO_WritePin(board_outputs_map[ch].port, board_outputs_map[ch].pin,
                          s_output_state[ch] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

uint32_t board_output_get_state(OutputChannel_t ch) {
    if (ch < OUTPUT_COUNT) {
        return s_output_state[ch] ? 1u : 0u;
    }

    return 0xFFFFFFFFu;
}
