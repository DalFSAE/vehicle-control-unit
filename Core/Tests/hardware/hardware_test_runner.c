#include "hardware_test_runner.h"
#include "stm32f4xx_hal.h"
#include "unity.h"
#include "board_outputs.h"

// ===========================================================================
// Hardware Test Runner
// Preforms suite of tests on vcu and vehicle, to very all subsystems are OK
// ===========================================================================


void setUp(void) {
    /* runs before each test */
}

void tearDown(void) {
    /* runs after each test */
}

void test_add_returns_correct_sum(void) {
    TEST_ASSERT_EQUAL_INT(5, (2+6));
}

void test_relays_click(void) {
    uint32_t stateRelay0 = board_output_get_state(OUTPUT_ALWAYS_ON);
    uint32_t stateRelay1 = board_output_get_state(OUTPUT_BRAKE_LIGHT);
    uint32_t stateRelay2 = board_output_get_state(OUTPUT_INVERTER);
    uint32_t stateRelay3 = board_output_get_state(OUTPUT_FANS);
    uint32_t stateRelay4 = board_output_get_state(OUTPUT_SDC);
    
    board_output_toggle(OUTPUT_ALWAYS_ON);
    HAL_Delay(50);
    board_output_toggle(OUTPUT_BRAKE_LIGHT);
    HAL_Delay(50);
    board_output_toggle(OUTPUT_INVERTER);
    HAL_Delay(50);
    board_output_toggle(OUTPUT_FANS);
    HAL_Delay(50);
    board_output_toggle(OUTPUT_SDC);
    
    stateRelay0 = board_output_get_state(OUTPUT_ALWAYS_ON);
    stateRelay1 = board_output_get_state(OUTPUT_BRAKE_LIGHT);
    stateRelay2 = board_output_get_state(OUTPUT_INVERTER);
    stateRelay3 = board_output_get_state(OUTPUT_FANS);
    stateRelay4 = board_output_get_state(OUTPUT_SDC);
}

uint32_t hardware_test_runner(uint32_t cmd) {
    (void)cmd;
    UNITY_BEGIN();
    RUN_TEST(test_add_returns_correct_sum);
    return UNITY_END();
}

