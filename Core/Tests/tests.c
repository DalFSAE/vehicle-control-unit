#include "tests.h"

#include "io_control.h"



void test_relays(void) {
    uint32_t stateRelay0 = relay_get_state(RELAY0);
    uint32_t stateRelay1 = relay_get_state(RELAY1);
    uint32_t stateRelay2 = relay_get_state(RELAY2);
    uint32_t stateRelay3 = relay_get_state(RELAY3);
    uint32_t stateRelay4 = relay_get_state(RELAY4);
    
    relay_toggle(RELAY0);
    osDelay(50);
    relay_toggle(RELAY1);
    osDelay(50);
    relay_toggle(RELAY2);
    osDelay(50);
    relay_toggle(RELAY3);
    osDelay(50);
    relay_toggle(RELAY4);
    
    stateRelay0 = relay_get_state(RELAY0);
    stateRelay1 = relay_get_state(RELAY1);
    stateRelay2 = relay_get_state(RELAY2);
    stateRelay3 = relay_get_state(RELAY3);
    stateRelay4 = relay_get_state(RELAY4);
}

