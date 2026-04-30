#include "hardware_test_runner.h"
#include "test_board_outputs.h"
#include "unity.h"
#include "board_outputs.h"
#include "cmsis_os2.h"

void setUp(void) {
    /* runs before each test */
}

void tearDown(void) {
    /* runs after each test */
}

// ===========================================================================
// Post-boot tests (OS scheduler running, osDelay available)
// ===========================================================================

static void test_brake_light_flash(void) {
    board_output_enable(OUTPUT_BRAKE_LIGHT);
    osDelay(200);
    TEST_ASSERT_EQUAL_UINT32(1u, board_output_get_state(OUTPUT_BRAKE_LIGHT));

    board_output_disable(OUTPUT_BRAKE_LIGHT);
    osDelay(200);
    TEST_ASSERT_EQUAL_UINT32(0u, board_output_get_state(OUTPUT_BRAKE_LIGHT));
}

static void test_debug_led_flash(void) {
    for (int i = 0; i < 10; i++) {
        for (int j = OUTPUT_DEBUG_LED3; j < 4; j++) {
            board_output_toggle(j);
            osDelay(100);
        }
    }
}

static void test_can_subsystem(void) {
    osDelay(1);
}

// ===========================================================================
// Runners
// ===========================================================================

uint32_t hardware_test_pre_boot(void) {
    UNITY_BEGIN();
    RUN_TEST(test_board_outputs_default_state);
    RUN_TEST(test_output_enable_disable);
    return (uint32_t)UNITY_END();
}

uint32_t hardware_test_post_boot(void) {
    UNITY_BEGIN();
    RUN_TEST(test_brake_light_flash);
    RUN_TEST(test_debug_led_flash);
    return (uint32_t)UNITY_END();
}

