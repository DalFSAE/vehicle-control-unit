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

static BootResult_t make_result(void) {
    return (BootResult_t){
        .tests_run = (uint16_t)Unity.NumberOfTests,
        .failures = (uint16_t)Unity.TestFailures,
        .ignored = (uint16_t)Unity.TestIgnores,
    };
}

BootResult_t hardware_test_pre_boot(void) {
    UNITY_BEGIN();
    RUN_TEST(test_board_outputs_default_state);
    RUN_TEST(test_output_enable_disable);
    UNITY_END();
    return make_result();
}

BootResult_t hardware_test_post_boot(void) {
    UNITY_BEGIN();
    RUN_TEST(test_brake_light_flash);
    RUN_TEST(test_debug_led_flash);
    UNITY_END();
    return make_result();
}
