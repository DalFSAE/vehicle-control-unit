#include "io_control.h"
#include "dms_defines.h"

// lib
#include "stdbool.h"
#include "main.h"
// Relay Controls

const RelayChannel_t relayState[RELAY_COUNT] = {0}; 


void relay_init(void) {
    return;
}

void relay_enable(RelayChannel_t ch) {
    return;
}

void relay_disable(RelayChannel_t ch) {
    return;
}

void relay_toggle(RelayChannel_t ch) {
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
    return;
}

uint32_t relay_get_state(RelayChannel_t ch) { 
    return -1;
}
