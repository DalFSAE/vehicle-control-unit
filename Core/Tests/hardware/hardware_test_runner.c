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
    for (int j = OUTPUT_DEBUG_LED3; j <= (int)OUTPUT_DEBUG_LED6; j++) {
        board_output_toggle((OutputChannel_t)j);
        osDelay(100);
    }
}

static void test_display_debug_binary(void) {
    for (int i = 0; i < 16; i++) {
        set_debug_led_value((uint8_t)i);
        osDelay(500);
    }
    set_debug_led_value(0);
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
    RUN_TEST(test_display_debug_binary);
    return (uint32_t)UNITY_END();
}

