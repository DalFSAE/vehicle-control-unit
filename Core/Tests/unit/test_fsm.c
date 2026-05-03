// test_fsm.c
// Unit tests for fsm.c using the Unity test framework
// fsm.c pulls in log.h and defines LOG_MODULE stub both out below before
// including the real headers so the translation unit compiles without the
// full firmware tree.

// Logging stubs
#ifndef LOG_H
#define LOG_H
#define LOG_SRC_FSM 0
#define LOG_LEVEL_ERROR 0
#define EVT_FAULT_SET 0
#define LOG_EVENT(lvl, evt, ...) ((void)0)
#endif

// Real firmware headers
#include "unity.h"
#include "vcu_io.h"
#include "fsm.h"

// Helpers

static VcuInputs make_clean_inputs(void) {
    VcuInputs i = {0};
    return i;
}

static VcuOutputs make_clean_outputs(void) {
    VcuOutputs o = {0};
    return o;
}

// Unity boilerplate
void setUp(void) {
}
void tearDown(void) {
}

// Transition table tests
// ST_ENTRY

void test_entry_transitions_to_standby(void) {
    // entry_state always returns FSM_EV_OK -> ST_STANDBY
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_STANDBY, step_fsm(ST_ENTRY, &cfg, &in, &out));
}

// ST_STANDBY

void test_standby_stays_when_no_conditions_met(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs(); // fwrd_switch=false, ts_active=false
    VcuOutputs       out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_STANDBY, step_fsm(ST_STANDBY, &cfg, &in, &out));
}

void test_standby_to_neutral_when_switch_and_ts_active(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_STANDBY, &cfg, &in, &out));
}

void test_standby_stays_when_only_switch_set(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = false;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_STANDBY, step_fsm(ST_STANDBY, &cfg, &in, &out));
}

void test_standby_stays_when_only_ts_active_set(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = false;
    in.ts_active = true;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_STANDBY, step_fsm(ST_STANDBY, &cfg, &in, &out));
}

// ST_NEUTRAL

void test_neutral_stays_on_no_input(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));
}

void test_neutral_to_forward_on_full_rtd_sequence(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.rtd_button = true;
    in.brake_pressed = true;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_NEUTRAL, &cfg, &in, &out));
}

void test_neutral_rtd_requires_all_three_conditions(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out;

    // switch + button, no brake
    in.fwrd_switch = true;
    in.rtd_button = true;
    in.brake_pressed = false;
    out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));

    // switch + brake, no button
    in.rtd_button = false;
    in.brake_pressed = true;
    out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));

    // button + brake, no switch
    in.fwrd_switch = false;
    in.rtd_button = true;
    out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));
}

void test_neutral_notready_path_not_reachable_via_step_fsm(void) {
    // neutral_state never emits FSM_EV_NOTREADY; the table entry exists but
    // is only reachable if a future state function is added that fires it.
    TEST_IGNORE_MESSAGE("ST_NEUTRAL->ST_STANDBY via FSM_EV_NOTREADY: no state fn emits it yet");
}

// ST_FORWARD

void test_forward_stays_when_healthy(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
}

void test_forward_to_neutral_when_switch_released(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = false;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
}

// ST_REVERSE
void test_reverse_stays_in_reverse(void) {
    // reverse_state is a stub returning FSM_EV_OK
    // [ST_REVERSE][FSM_EV_OK] -> ST_REVERSE
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_REVERSE, step_fsm(ST_REVERSE, &cfg, &in, &out));
}

// Output signal tests
void test_entry_sets_all_relays_and_watchdog(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    step_fsm(ST_ENTRY, &cfg, &in, &out);
    TEST_ASSERT_TRUE(out.can_watchdog);
    TEST_ASSERT_TRUE(out.tssi_en);
    TEST_ASSERT_TRUE(out.relay_always_on);
    TEST_ASSERT_TRUE(out.relay_inverter);
}

void test_standby_inverter_off_throttle_disabled(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    step_fsm(ST_STANDBY, &cfg, &in, &out);
    TEST_ASSERT_FALSE(out.relay_inverter);
    TEST_ASSERT_FALSE(out.throttle_enabled);
    TEST_ASSERT_TRUE(out.relay_always_on);
    TEST_ASSERT_TRUE(out.can_watchdog);
    TEST_ASSERT_FALSE(out.tssi_en);
}

void test_neutral_inverter_on_throttle_disabled(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    step_fsm(ST_NEUTRAL, &cfg, &in, &out);
    TEST_ASSERT_TRUE(out.relay_inverter);
    TEST_ASSERT_FALSE(out.throttle_enabled);
    TEST_ASSERT_FALSE(out.tssi_en);
}

void test_neutral_rtd_fires_buzzer_and_sets_forward_direction(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.rtd_button = true;
    in.brake_pressed = true;
    VcuOutputs out = make_clean_outputs();
    step_fsm(ST_NEUTRAL, &cfg, &in, &out);
    TEST_ASSERT_EQUAL(1000, out.buzzer_beep_ms);
    TEST_ASSERT_EQUAL(MOTOR_DIR_FORWARD, out.motor_direction);
}

void test_forward_enables_throttle_when_healthy(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    VcuOutputs out = make_clean_outputs();
    step_fsm(ST_FORWARD, &cfg, &in, &out);
    TEST_ASSERT_TRUE(out.throttle_enabled);
}

void test_forward_disables_throttle_when_switch_released(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = false;
    VcuOutputs out = make_clean_outputs();
    out.throttle_enabled = true;
    step_fsm(ST_FORWARD, &cfg, &in, &out);
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

// Forward state - fault handling

// FAULT_APPS_DISAGREE
void test_forward_apps_disagree_cut_throttle_stays_forward(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.apps_disagree = FAULT_RESP_CUT_THROTTLE;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_APPS_DISAGREE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_forward_apps_disagree_return_neutral_goes_to_neutral(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.apps_disagree = FAULT_RESP_RETURN_NEUTRAL;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_APPS_DISAGREE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

// FAULT_PEDAL_PLAUS

void test_forward_pedal_plaus_cut_throttle_stays_forward(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.pedal_plaus = FAULT_RESP_CUT_THROTTLE;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_PEDAL_PLAUS;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_forward_pedal_plaus_return_neutral_goes_to_neutral(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.pedal_plaus = FAULT_RESP_RETURN_NEUTRAL;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_PEDAL_PLAUS;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
}

// FAULT_SENSOR_RANGE

void test_forward_sensor_range_cut_throttle_stays_forward(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.sensor_range = FAULT_RESP_CUT_THROTTLE;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_SENSOR_RANGE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_forward_sensor_range_return_neutral_goes_to_neutral(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.sensor_range = FAULT_RESP_RETURN_NEUTRAL;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_SENSOR_RANGE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
}

// Fault priority: apps_disagree is checked first in forward_state

void test_forward_apps_disagree_checked_before_pedal_plaus(void) {
    // apps_disagree = CUT_THROTTLE (stay forward)
    // pedal_plaus   = RETURN_NEUTRAL (would go neutral if checked first)
    // Expected: apps_disagree fires first -> ST_FORWARD
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.apps_disagree = FAULT_RESP_CUT_THROTTLE;
    cfg.pedal_plaus = FAULT_RESP_RETURN_NEUTRAL;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_APPS_DISAGREE | FAULT_PEDAL_PLAUS;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
}

void test_forward_pedal_plaus_checked_before_sensor_range(void) {
    // pedal_plaus  = CUT_THROTTLE (stay forward)
    // sensor_range = RETURN_NEUTRAL (would go neutral if checked first)
    // Expected: pedal_plaus fires first -> ST_FORWARD
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.pedal_plaus = FAULT_RESP_CUT_THROTTLE;
    cfg.sensor_range = FAULT_RESP_RETURN_NEUTRAL;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_PEDAL_PLAUS | FAULT_SENSOR_RANGE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
}

void test_forward_switch_release_checked_before_faults(void) {
    // Even with an active fault that would return neutral,
    // the switch-release path fires FSM_EV_STOP first (same destination,
    // but verifies ordering does not accidentally skip the switch check).
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.apps_disagree = FAULT_RESP_RETURN_NEUTRAL;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = false;
    in.fault_flags = FAULT_APPS_DISAGREE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_forward_fault_none_enables_throttle(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.fault_flags = FAULT_NONE;
    VcuOutputs out = make_clean_outputs();
    step_fsm(ST_FORWARD, &cfg, &in, &out);
    TEST_ASSERT_TRUE(out.throttle_enabled);
}

// ===========================================================================
// FaultConfig_default
// ===========================================================================

void test_fault_config_default_apps_disagree(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    TEST_ASSERT_EQUAL(FAULT_RESP_CUT_THROTTLE, cfg.apps_disagree);
}

void test_fault_config_default_pedal_plaus(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    TEST_ASSERT_EQUAL(FAULT_RESP_RETURN_NEUTRAL, cfg.pedal_plaus);
}

void test_fault_config_default_sensor_range(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    TEST_ASSERT_EQUAL(FAULT_RESP_RETURN_NEUTRAL, cfg.sensor_range);
}

void test_fault_config_default_can_timeout(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    TEST_ASSERT_EQUAL(FAULT_RESP_RETURN_NEUTRAL, cfg.can_timeout);
}

// ===========================================================================
// Entry point
// ===========================================================================

int main(void) {
    UNITY_BEGIN();

    // Transitions
    RUN_TEST(test_entry_transitions_to_standby);
    RUN_TEST(test_standby_stays_when_no_conditions_met);
    RUN_TEST(test_standby_to_neutral_when_switch_and_ts_active);
    RUN_TEST(test_standby_stays_when_only_switch_set);
    RUN_TEST(test_standby_stays_when_only_ts_active_set);
    RUN_TEST(test_neutral_stays_on_no_input);
    RUN_TEST(test_neutral_to_forward_on_full_rtd_sequence);
    RUN_TEST(test_neutral_rtd_requires_all_three_conditions);
    RUN_TEST(test_neutral_notready_path_not_reachable_via_step_fsm);
    RUN_TEST(test_forward_stays_when_healthy);
    RUN_TEST(test_forward_to_neutral_when_switch_released);
    RUN_TEST(test_reverse_stays_in_reverse);

    // Output signals
    RUN_TEST(test_entry_sets_all_relays_and_watchdog);
    RUN_TEST(test_standby_inverter_off_throttle_disabled);
    RUN_TEST(test_neutral_inverter_on_throttle_disabled);
    RUN_TEST(test_neutral_rtd_fires_buzzer_and_sets_forward_direction);
    RUN_TEST(test_forward_enables_throttle_when_healthy);
    RUN_TEST(test_forward_disables_throttle_when_switch_released);

    // Forward fault handling
    RUN_TEST(test_forward_apps_disagree_cut_throttle_stays_forward);
    RUN_TEST(test_forward_apps_disagree_return_neutral_goes_to_neutral);
    RUN_TEST(test_forward_pedal_plaus_cut_throttle_stays_forward);
    RUN_TEST(test_forward_pedal_plaus_return_neutral_goes_to_neutral);
    RUN_TEST(test_forward_sensor_range_cut_throttle_stays_forward);
    RUN_TEST(test_forward_sensor_range_return_neutral_goes_to_neutral);
    RUN_TEST(test_forward_apps_disagree_checked_before_pedal_plaus);
    RUN_TEST(test_forward_pedal_plaus_checked_before_sensor_range);
    RUN_TEST(test_forward_switch_release_checked_before_faults);
    RUN_TEST(test_forward_fault_none_enables_throttle);

    // FaultConfig_default
    RUN_TEST(test_fault_config_default_apps_disagree);
    RUN_TEST(test_fault_config_default_pedal_plaus);
    RUN_TEST(test_fault_config_default_sensor_range);
    RUN_TEST(test_fault_config_default_can_timeout);

    return UNITY_END();
}
