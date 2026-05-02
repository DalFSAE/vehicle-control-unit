#include "test_fsm_hil.h"
#include "fsm_test_helpers.h"
#include "input_control.h"
#include "unity.h"
#include "fsm.h"
#include "fsm_task.h"
#include "vehicle_state.h"
#include "vcu_io.h"
#include "board_outputs.h"
#include "cmsis_os2.h"
#include "dio.h"

// ===========================================================================
// Tests
// ===========================================================================

void test_fsm_in_standby_after_boot(void) {
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);
}

void test_fsm_standby_requires_switch_and_ts(void) {
    suspend_sensor();

    clear_inputs();
    g_spoof.fwrd_switch = true;
    g_spoof.ts_active   = false;
    vcu_spoof_inputs(&g_spoof);
    osDelay(FSM_SETTLE_MS);
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);

    clear_inputs();
    g_spoof.fwrd_switch = false;
    g_spoof.ts_active   = true;
    vcu_spoof_inputs(&g_spoof);
    osDelay(FSM_SETTLE_MS);
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);

    clear_inputs();
    resume_sensor();
}

void test_fsm_transitions_to_neutral(void) {
    suspend_sensor();
    walk_to_neutral();

    TEST_ASSERT_EQUAL(ST_NEUTRAL, g_fsm_state);
    TEST_ASSERT_EQUAL(1u, board_output_get_state(OUTPUT_INVERTER));

    clear_inputs();
    resume_sensor();
}

void test_fsm_rtd_requires_brake(void) {
    suspend_sensor();
    walk_to_neutral();

    g_spoof.fwrd_switch   = true;
    g_spoof.brake_pressed = false;
    g_spoof.rtd_button    = true;
    vcu_spoof_inputs(&g_spoof);
    osDelay(FSM_SETTLE_MS);
    g_spoof.rtd_button = false;
    vcu_spoof_inputs(&g_spoof);

    TEST_ASSERT_EQUAL(ST_NEUTRAL, g_fsm_state);

    clear_inputs();
    resume_sensor();
}

void test_fsm_rtd_with_brake_enters_forward(void) {
    suspend_sensor();
    walk_to_neutral();
    walk_to_forward();

    TEST_ASSERT_EQUAL(ST_FORWARD, g_fsm_state);

    clear_inputs();
    resume_sensor();
}

void test_fsm_pedal_plaus_returns_to_neutral(void) {
    suspend_sensor();
    walk_to_neutral();
    walk_to_forward();

    g_spoof.fwrd_switch = true;
    g_spoof.fault_flags = FAULT_PEDAL_PLAUS;
    vcu_spoof_inputs(&g_spoof);
    osDelay(FSM_SETTLE_MS);

    TEST_ASSERT_EQUAL(ST_NEUTRAL, g_fsm_state);
    TEST_ASSERT_EQUAL(1u, board_output_get_state(OUTPUT_INVERTER));

    clear_inputs();
    resume_sensor();
}

void test_if_debug_button_changes_state(void) {
    suspend_sensor();
    walk_to_neutral();

    g_spoof.fwrd_switch   = true;
    g_spoof.brake_pressed = true;
    bool button_was_pressed = false;

    for (int i = 0; i < 20; i++) {
        g_spoof.rtd_button = read_pcb_user_button();
        button_was_pressed |= g_spoof.rtd_button;
        vcu_spoof_inputs(&g_spoof);
        osDelay(100);
        if (g_fsm_state == ST_FORWARD)
            break;
    }

    if (!button_was_pressed) {
        TEST_IGNORE_MESSAGE("RTD button was not pressed during the test window");
    } else {
        TEST_ASSERT_EQUAL_MESSAGE(ST_FORWARD, g_fsm_state,
            "FSM did not transition to FORWARD state after RTD button was pressed");
    }

    clear_inputs();
    resume_sensor();
}

void test_throttle_step_response_from_nuetral(void) {
    suspend_sensor();
    walk_to_neutral();

    g_spoof.fwrd_switch   = false;
    g_spoof.brake_pressed = false;
    g_spoof.rtd_button    = true;
    vcu_spoof_inputs(&g_spoof);
    osDelay(FSM_SETTLE_MS);
    g_spoof.rtd_button = false;

    ramp_throttle(0.0f, 1.0f, 10, FSM_SETTLE_MS);
    ramp_throttle(1.0f, 0.0f, 10, FSM_SETTLE_MS);

    clear_inputs();
    resume_sensor();
}

void test_throttle_step_response_from_forward(void) {
    suspend_sensor();
    walk_to_neutral();
    walk_to_forward();

    ramp_throttle(0.0f, 1.0f, 10, FSM_SETTLE_MS);
    ramp_throttle(1.0f, 0.0f, 10, FSM_SETTLE_MS);

    clear_inputs();
    resume_sensor();
}
