#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OUTPUT_DEBUG_LED4,
    OUTPUT_DEBUG_LED3,
    OUTPUT_DEBUG_LED5,
    OUTPUT_DEBUG_LED6,
    OUTPUT_ALWAYS_ON,
    OUTPUT_BRAKE_LIGHT,
    OUTPUT_INVERTER,
    OUTPUT_FANS,
    OUTPUT_SDC,
    OUTPUT_AUX,
    OUTPUT_COUNT     // must be at end
} OutputChannel_t;

void board_outputs_init(void);
void board_output_set(OutputChannel_t ch, bool value);
void board_output_enable(OutputChannel_t ch);
void board_output_disable(OutputChannel_t ch);
void board_output_toggle(OutputChannel_t ch);
uint32_t board_output_get_state(OutputChannel_t ch);
