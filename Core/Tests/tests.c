#include "tests.h"

#include "io_control.h"



void test_relays(void) {
    uint32_t stateRelay0 = relay_get_state(RELAY_ALWAYS_ON);
    uint32_t stateRelay1 = relay_get_state(RELAY_BRAKE_LIGHT);
    uint32_t stateRelay2 = relay_get_state(RELAY_INVERTER);
    uint32_t stateRelay3 = relay_get_state(RELAY_FANS);
    uint32_t stateRelay4 = relay_get_state(RELAY_SDC);
    
    relay_toggle(RELAY_ALWAYS_ON);
    osDelay(50);
    relay_toggle(RELAY_BRAKE_LIGHT);
    osDelay(50);
    relay_toggle(RELAY_INVERTER);
    osDelay(50);
    relay_toggle(RELAY_FANS);
    osDelay(50);
    relay_toggle(RELAY_SDC);
    
    stateRelay0 = relay_get_state(RELAY_ALWAYS_ON);
    stateRelay1 = relay_get_state(RELAY_BRAKE_LIGHT);
    stateRelay2 = relay_get_state(RELAY_INVERTER);
    stateRelay3 = relay_get_state(RELAY_FANS);
    stateRelay4 = relay_get_state(RELAY_SDC);
}

