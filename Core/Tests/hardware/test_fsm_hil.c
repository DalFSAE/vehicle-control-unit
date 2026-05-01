#include "test_fsm_hil.h"
#include "input_control.h"
#include "unity.h"
#include "fsm.h"
#include "fsm_task.h"
#include "sensor_control.h"
#include "vehicle_state.h"
#include "board_outputs.h"
#include "cmsis_os2.h"

#include "dio.h"

// Wait long enough for the FSM to complete several cycles after an input change.
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

static void clear_inputs(void) {
    g_vcu.throttle_request = 0.0f;
    g_vcu.brake_pressed = false;
    g_vcu.rtd_button = false;
    g_vcu.fwrd_switch = false;
    g_vcu.fault_flags = 0u;
    g_can.ts_active = false;
}

// Drive FSM from STANDBY to NEUTRAL. Assumes sensor is already suspended.
static void walk_to_neutral(void) {
    clear_inputs();
    g_vcu.fwrd_switch = true;
    g_can.ts_active = true;
    osDelay(FSM_SETTLE_MS);
}

// Drive FSM from NEUTRAL to FORWARD. Assumes sensor is suspended and FSM is in
// NEUTRAL. rtd_button is pulsed; rising_edge() in vcu_read_inputs handles the edge.
static void walk_to_forward(void) {
    g_vcu.fwrd_switch = true;
    g_vcu.brake_pressed = true;
    g_vcu.rtd_button = true;
    osDelay(FSM_SETTLE_MS);
    g_vcu.rtd_button = false;
}

// ===========================================================================
// Tests
// ===========================================================================

// FSM should be in STANDBY after normal boot (boot sequence puts it there
// before the hardware test task runs).
void test_fsm_in_standby_after_boot(void) {
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);
}

// STANDBY only advances to NEUTRAL when BOTH fwrd_switch AND ts_active are set.
void test_fsm_standby_requires_switch_and_ts(void) {
    suspend_sensor();

    clear_inputs();
    g_vcu.fwrd_switch = true;
    g_can.ts_active = false;
    osDelay(FSM_SETTLE_MS);
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);

    clear_inputs();
    g_vcu.fwrd_switch = false;
    g_can.ts_active = true;
    osDelay(FSM_SETTLE_MS);
    TEST_ASSERT_EQUAL(ST_STANDBY, g_fsm_state);

    clear_inputs();
    resume_sensor();
}

// Confirm STANDBY → NEUTRAL transition and that the inverter relay comes on.
void test_fsm_transitions_to_neutral(void) {
    suspend_sensor();
    walk_to_neutral();

    TEST_ASSERT_EQUAL(ST_NEUTRAL, g_fsm_state);
    TEST_ASSERT_EQUAL(1u, board_output_get_state(OUTPUT_INVERTER));

    clear_inputs();
    resume_sensor();
}

// RTD without brake must keep the FSM in NEUTRAL.
void test_fsm_rtd_requires_brake(void) {
    suspend_sensor();
    walk_to_neutral();

    g_vcu.fwrd_switch = true;
    g_vcu.brake_pressed = false;
    g_vcu.rtd_button = true;
    osDelay(FSM_SETTLE_MS);
    g_vcu.rtd_button = false;

    TEST_ASSERT_EQUAL(ST_NEUTRAL, g_fsm_state);

    clear_inputs();
    resume_sensor();
}

// RTD with all three conditions (switch + brake + button edge) enters FORWARD.
void test_fsm_rtd_with_brake_enters_forward(void) {
    suspend_sensor();
    walk_to_neutral();
    walk_to_forward();

    TEST_ASSERT_EQUAL(ST_FORWARD, g_fsm_state);

    clear_inputs();
    resume_sensor();
}

// PEDAL_PLAUS fault while in FORWARD must return FSM to NEUTRAL.
// Default fault config maps pedal_plaus -> FAULT_RESP_RETURN_NEUTRAL.
void test_fsm_pedal_plaus_returns_to_neutral(void) {
    suspend_sensor();
    walk_to_neutral();
    walk_to_forward();

    g_vcu.fwrd_switch = true;
    g_vcu.fault_flags = FAULT_PEDAL_PLAUS;
    osDelay(FSM_SETTLE_MS);

    TEST_ASSERT_EQUAL(ST_NEUTRAL, g_fsm_state);
    // Inverter stays on in NEUTRAL. Confirms we landed in NEUTRAL not STANDBY.
    TEST_ASSERT_EQUAL(1u, board_output_get_state(OUTPUT_INVERTER));

    clear_inputs();
    resume_sensor();
}

// Requires user to physically press the RTD button during the polling window.
// Fails if no press is detected within the timeout.
void test_if_debug_button_changes_state(void) {
    suspend_sensor();
    walk_to_neutral();

    g_vcu.fwrd_switch = true;
    g_vcu.brake_pressed = true;

    for (int i = 0; i < 20; i++) {
        g_vcu.rtd_button = read_pcb_user_button();
        osDelay(100);
        if (g_fsm_state == ST_FORWARD) {
            break;
        }
    }

    TEST_ASSERT_EQUAL_MESSAGE(ST_FORWARD, g_fsm_state, "RTD button was not pressed during the test window");

    clear_inputs();
    resume_sensor();
}