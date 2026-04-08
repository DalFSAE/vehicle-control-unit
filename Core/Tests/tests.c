#include "tests.h"

#include "cmsis_os2.h"
#include "io_control.h"
#include "board_outputs.h"



void test_relays(void) {
    uint32_t stateRelay0 = board_output_get_state(OUTPUT_ALWAYS_ON);
    uint32_t stateRelay1 = board_output_get_state(OUTPUT_BRAKE_LIGHT);
    uint32_t stateRelay2 = board_output_get_state(OUTPUT_INVERTER);
    uint32_t stateRelay3 = board_output_get_state(OUTPUT_FANS);
    uint32_t stateRelay4 = board_output_get_state(OUTPUT_SDC);
    
    board_output_toggle(OUTPUT_ALWAYS_ON);
    osDelay(50);
    board_output_toggle(OUTPUT_BRAKE_LIGHT);
    osDelay(50);
    board_output_toggle(OUTPUT_INVERTER);
    osDelay(50);
    board_output_toggle(OUTPUT_FANS);
    osDelay(50);
    board_output_toggle(OUTPUT_SDC);
    
    stateRelay0 = board_output_get_state(OUTPUT_ALWAYS_ON);
    stateRelay1 = board_output_get_state(OUTPUT_BRAKE_LIGHT);
    stateRelay2 = board_output_get_state(OUTPUT_INVERTER);
    stateRelay3 = board_output_get_state(OUTPUT_FANS);
    stateRelay4 = board_output_get_state(OUTPUT_SDC);
}
