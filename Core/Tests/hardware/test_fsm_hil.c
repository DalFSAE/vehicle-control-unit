#include "test_fsm_hil.h"
#include "input_control.h"
#include "unity.h"
#include "fsm.h"
#include "fsm_task.h"
#include "vehicle_state.h"
#include "vcu_io.h"
#include "board_outputs.h"
#include "sensor_control.h"
#include "cmsis_os2.h"
#include "dio.h"

#include <string.h>

#define FSM_SETTLE_MS (FSM_PERIOD_MS * 4u)

// ===========================================================================
// Helpers
// ===========================================================================

static void suspend_sensor(void) {
    osThreadSuspend(sensor_task_get_handle());
}
static void resume_sensor(void) {
    osThreadResume(sensor_task_get_handle());
}

static VcuInputs s_spoof = {0};

static void clear_inputs(void) {
    memset(&s_spoof, 0, sizeof(s_spoof));
    vcu_spoof_inputs(&s_spoof);
}

// Drive FSM from STANDBY to NEUTRAL. Assumes sensor is already suspended.
static void walk_to_neutral(void) {
    clear_inputs();
    s_spoof.fwrd_switch = true;
    s_spoof.ts_active = true;
    vcu_spoof_inputs(&s_spoof);
    osDelay(FSM_SETTLE_MS);
}

// Drive FSM from NEUTRAL to FORWARD.
static void walk_to_forward(void) {
    s_spoof.fwrd_switch = true;
    s_spoof.brake_pressed = true;
    s_spoof.rtd_button = true;
    vcu_spoof_inputs(&s_spoof);
    osDelay(FSM_SETTLE_MS);
    s_spoof.rtd_button = false;
    vcu_spoof_inputs(&s_spoof);
}

static void ramp_throttle(float start, float end, uint32_t steps, uint32_t delta_ms) {
    for (uint32_t i = 0; i <= steps; i++) {
        s_spoof.throttle_request = start + (end - start) * (i / (float)steps);
        vcu_spoof_inputs(&s_spoof);
        osDelay(delta_ms);
    }
}

// ===========================================================================
// Tests
// ===========================================================================

void test_fsm_in_standby_after_boot(void) {
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);
}

void test_fsm_standby_requires_switch_and_ts(void) {
    suspend_sensor();

    clear_inputs();
    s_spoof.fwrd_switch = true;
    s_spoof.ts_active = false;
    vcu_spoof_inputs(&s_spoof);
    osDelay(FSM_SETTLE_MS);
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);

    clear_inputs();
    s_spoof.fwrd_switch = false;
    s_spoof.ts_active = true;
    vcu_spoof_inputs(&s_spoof);
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

    s_spoof.fwrd_switch = true;
    s_spoof.brake_pressed = false;
    s_spoof.rtd_button = true;
    vcu_spoof_inputs(&s_spoof);
    osDelay(FSM_SETTLE_MS);
    s_spoof.rtd_button = false;
    vcu_spoof_inputs(&s_spoof);

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

    s_spoof.fwrd_switch = true;
    s_spoof.fault_flags = FAULT_PEDAL_PLAUS;
    vcu_spoof_inputs(&s_spoof);
    osDelay(FSM_SETTLE_MS);

    TEST_ASSERT_EQUAL(ST_NEUTRAL, g_fsm_state);
    TEST_ASSERT_EQUAL(1u, board_output_get_state(OUTPUT_INVERTER));

    clear_inputs();
    resume_sensor();
}

void test_if_debug_button_changes_state(void) {
    suspend_sensor();
    walk_to_neutral();

    s_spoof.fwrd_switch = true;
    s_spoof.brake_pressed = true;
    bool button_was_pressed = false;

    for (int i = 0; i < 20; i++) {
        s_spoof.rtd_button = read_pcb_user_button();
        button_was_pressed |= s_spoof.rtd_button;
        vcu_spoof_inputs(&s_spoof);
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

    s_spoof.fwrd_switch = false;
    s_spoof.brake_pressed = false;
    s_spoof.rtd_button = true;
    vcu_spoof_inputs(&s_spoof);
    osDelay(FSM_SETTLE_MS);
    s_spoof.rtd_button = false;

    for (int i = 0; i <= 10; i++) {
        s_spoof.throttle_request = i / 10.0f;
        vcu_spoof_inputs(&s_spoof);
        osDelay(FSM_SETTLE_MS);
    }
    for (int i = 9; i >= 0; i--) {
        s_spoof.throttle_request = i / 10.0f;
        vcu_spoof_inputs(&s_spoof);
        osDelay(FSM_SETTLE_MS);
    }

    clear_inputs();
    resume_sensor();
}
