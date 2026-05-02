// hardware_test_runner.c
#include "hardware_test_runner.h"
#include "unity.h"
#include "board_outputs.h"
#include "cmsis_os2.h"
#include "log.h"
#include "unity_internals.h"
#include "test_board_outputs.h"
#include "test_fsm_hil.h"
#include "test_can.h"
#include "vcu_io.h"

void setUp(void) {
    /* runs before each test */
}

void tearDown(void) {
    /* runs after each test */
}

// ===========================================================================
// Post-boot tests (OS scheduler running, osDelay available)
// ===========================================================================

static void test_debug_led_flash(void) {
    for (int i = 0; i < 10; i++) {
        for (int j = OUTPUT_DEBUG_LED3; j < 4; j++) {
            board_output_toggle(j);
            osDelay(100);
        }
    }
}

static void run_fsm_tests(void) {
    RUN_TEST(test_debug_led_flash);
    RUN_TEST(test_fsm_in_standby_after_boot);
    RUN_TEST(test_fsm_standby_requires_switch_and_ts);
    RUN_TEST(test_fsm_transitions_to_neutral);
    RUN_TEST(test_fsm_rtd_requires_brake);
    RUN_TEST(test_fsm_rtd_with_brake_enters_forward);
    RUN_TEST(test_fsm_pedal_plaus_returns_to_neutral);
    RUN_TEST(test_if_debug_button_changes_state); // optional, requires user input
}


// ===========================================================================
// Runners
// ===========================================================================

BootResult_t make_result(void) {
    return (BootResult_t){
        .tests_run = (uint16_t)Unity.NumberOfTests,
        .failures = (uint16_t)Unity.TestFailures,
        .ignored = (uint16_t)Unity.TestIgnores,
    };
}

BootResult_t hardware_test_pre_boot(void) {
    log_printf("===hardware_test_pre_boot===\r\n");
    UNITY_BEGIN();
    RUN_TEST(test_board_outputs_default_state);
    RUN_TEST(test_output_enable_disable);
    UNITY_END();
    return make_result();
}

BootResult_t hardware_test_post_boot(void) {
    UNITY_BEGIN();
    run_fsm_tests();
    UNITY_END();
    return make_result();
}

void hardware_post_test_task(void *argument) {
    (void)argument;
    log_printf("===BEGIN_HIL_TESTS===\r\n");
    hardware_test_post_boot();
    log_printf("===BEGIN_CAN_TESTS===\r\n");
    run_can_tests();
    vcu_clear_spoof();
    osThreadExit();
}