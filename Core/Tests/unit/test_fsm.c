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

void test_neutral_stays_when_healthy_but_no_rtd(void) {
    // fwrd_switch + ts_active = healthy; no button/brake -> no RTD -> stay NEUTRAL
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active   = true;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));
}

void test_neutral_to_forward_on_full_rtd_sequence(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
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
    in.ts_active = true;
    in.rtd_button = true;
    in.brake_pressed = false;
    out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));

    // switch + brake, no button
    in.rtd_button = false;
    in.brake_pressed = true;
    out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));

    // no switch (ts also false) -> NOTREADY fires before RTD is checked -> STANDBY
    in.fwrd_switch = false;
    in.ts_active = false;
    in.rtd_button = true;
    out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_STANDBY, step_fsm(ST_NEUTRAL, &cfg, &in, &out));
}

void test_neutral_notready_when_switch_released(void) {
    // neutral_state emits FSM_EV_NOTREADY when fwrd_switch drops -> ST_STANDBY
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = false;
    in.ts_active = true;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_STANDBY, step_fsm(ST_NEUTRAL, &cfg, &in, &out));
}

void test_neutral_stays_when_ts_active_lost_but_switch_held(void) {
    // ts_active loss in NEUTRAL does not drop to STANDBY; only switch release does.
    // STANDBY = inverter off; NEUTRAL = inverter on, waiting for RTD.
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = false;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_NEUTRAL, &cfg, &in, &out));
}

// ST_FORWARD

void test_forward_stays_when_healthy(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
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
    // When VCU_ENABLE_REVERSE=0 (default), reverse_state returns FSM_EV_OK
    // and [ST_REVERSE][FSM_EV_OK] -> ST_REVERSE (locked-out self-loop).
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_REVERSE, step_fsm(ST_REVERSE, &cfg, &in, &out));
}

// ST_FAULT

void test_fault_state_latches(void) {
    // ST_FAULT stays in ST_FAULT for every event.
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FAULT, step_fsm(ST_FAULT, &cfg, &in, &out));
}

void test_fault_state_disables_throttle_and_inverter(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    step_fsm(ST_FAULT, &cfg, &in, &out);
    TEST_ASSERT_FALSE(out.throttle_enabled);
    TEST_ASSERT_FALSE(out.relay_inverter);
    TEST_ASSERT_TRUE(out.sdc_open);
    TEST_ASSERT_TRUE(out.relay_always_on);
}

void test_fault_state_stays_latched_even_with_healthy_inputs(void) {
    // All healthy inputs: ST_FAULT should not escape.
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FAULT, step_fsm(ST_FAULT, &cfg, &in, &out));
}

void test_latch_fault_response_goes_to_fault_state(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.apps_disagree = FAULT_RESP_LATCH_FAULT;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
    in.fault_flags = FAULT_APPS_DISAGREE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FAULT, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_TRUE(out.sdc_open);
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_sdc_open_response_opens_sdc_and_goes_to_neutral(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.apps_disagree = FAULT_RESP_SDC_OPEN;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
    in.fault_flags = FAULT_APPS_DISAGREE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_TRUE(out.sdc_open);
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

// Output signal tests
void test_entry_sets_all_relays_and_watchdog(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    step_fsm(ST_ENTRY, &cfg, &in, &out);
    TEST_ASSERT_FALSE(out.can_watchdog);
    TEST_ASSERT_TRUE(out.relay_always_on);
    TEST_ASSERT_FALSE(out.relay_inverter);
}

void test_standby_inverter_off_throttle_disabled(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    step_fsm(ST_STANDBY, &cfg, &in, &out);
    TEST_ASSERT_FALSE(out.relay_inverter);
    TEST_ASSERT_FALSE(out.throttle_enabled);
    TEST_ASSERT_TRUE(out.relay_always_on);
}

void test_neutral_inverter_on_throttle_disabled(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    VcuOutputs       out = make_clean_outputs();
    step_fsm(ST_NEUTRAL, &cfg, &in, &out);
    TEST_ASSERT_TRUE(out.relay_inverter);
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_neutral_rtd_fires_buzzer_and_sets_forward_direction(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    VcuInputs        in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
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
    in.ts_active = true;
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
    in.ts_active = true;
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
    in.ts_active = true;
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
    in.ts_active = true;
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
    in.ts_active = true;
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
    in.ts_active = true;
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
    in.ts_active = true;
    in.fault_flags = FAULT_SENSOR_RANGE;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
}

// FAULT_CAN_TIMEOUT (#94)

void test_forward_can_timeout_cut_throttle_stays_forward(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.can_timeout = FAULT_RESP_CUT_THROTTLE;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
    in.fault_flags = FAULT_CAN_TIMEOUT;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_forward_can_timeout_return_neutral_goes_to_neutral(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.can_timeout = FAULT_RESP_RETURN_NEUTRAL;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = true;
    in.fault_flags = FAULT_CAN_TIMEOUT;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

// ts_active loss in ST_FORWARD (#97)

void test_forward_ts_active_loss_return_neutral(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    // Default: ts_lost = FAULT_RESP_RETURN_NEUTRAL
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = false;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_NEUTRAL, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_forward_ts_active_loss_cut_throttle_stays_forward(void) {
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.ts_lost = FAULT_RESP_CUT_THROTTLE;
    VcuInputs in = make_clean_inputs();
    in.fwrd_switch = true;
    in.ts_active = false;
    VcuOutputs out = make_clean_outputs();
    TEST_ASSERT_EQUAL(ST_FORWARD, step_fsm(ST_FORWARD, &cfg, &in, &out));
    TEST_ASSERT_FALSE(out.throttle_enabled);
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
    in.ts_active = true;
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
    in.ts_active = true;
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
    in.ts_active = true;
    in.fault_flags = FAULT_NONE;
    VcuOutputs out = make_clean_outputs();
    step_fsm(ST_FORWARD, &cfg, &in, &out);
    TEST_ASSERT_TRUE(out.throttle_enabled);
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
    RUN_TEST(test_neutral_stays_when_healthy_but_no_rtd);
    RUN_TEST(test_neutral_to_forward_on_full_rtd_sequence);
    RUN_TEST(test_neutral_rtd_requires_all_three_conditions);
    RUN_TEST(test_neutral_notready_when_switch_released);
    RUN_TEST(test_neutral_stays_when_ts_active_lost_but_switch_held);
    RUN_TEST(test_forward_stays_when_healthy);
    RUN_TEST(test_forward_to_neutral_when_switch_released);
    RUN_TEST(test_reverse_stays_in_reverse);
    RUN_TEST(test_fault_state_latches);
    RUN_TEST(test_fault_state_stays_latched_even_with_healthy_inputs);

    // Output signals
    RUN_TEST(test_entry_sets_all_relays_and_watchdog);
    RUN_TEST(test_standby_inverter_off_throttle_disabled);
    RUN_TEST(test_neutral_inverter_on_throttle_disabled);
    RUN_TEST(test_neutral_rtd_fires_buzzer_and_sets_forward_direction);
    RUN_TEST(test_forward_enables_throttle_when_healthy);
    RUN_TEST(test_forward_disables_throttle_when_switch_released);
    RUN_TEST(test_fault_state_disables_throttle_and_inverter);

    // Forward fault handling
    RUN_TEST(test_forward_apps_disagree_cut_throttle_stays_forward);
    RUN_TEST(test_forward_apps_disagree_return_neutral_goes_to_neutral);
    RUN_TEST(test_forward_pedal_plaus_cut_throttle_stays_forward);
    RUN_TEST(test_forward_pedal_plaus_return_neutral_goes_to_neutral);
    RUN_TEST(test_forward_sensor_range_cut_throttle_stays_forward);
    RUN_TEST(test_forward_sensor_range_return_neutral_goes_to_neutral);
    RUN_TEST(test_forward_can_timeout_cut_throttle_stays_forward);
    RUN_TEST(test_forward_can_timeout_return_neutral_goes_to_neutral);
    RUN_TEST(test_forward_ts_active_loss_return_neutral);
    RUN_TEST(test_forward_ts_active_loss_cut_throttle_stays_forward);
    RUN_TEST(test_latch_fault_response_goes_to_fault_state);
    RUN_TEST(test_sdc_open_response_opens_sdc_and_goes_to_neutral);
    RUN_TEST(test_forward_apps_disagree_checked_before_pedal_plaus);
    RUN_TEST(test_forward_pedal_plaus_checked_before_sensor_range);
    RUN_TEST(test_forward_switch_release_checked_before_faults);
    RUN_TEST(test_forward_fault_none_enables_throttle);

    // FaultConfig_default
    RUN_TEST(test_fault_config_default_apps_disagree);
    RUN_TEST(test_fault_config_default_pedal_plaus);
    RUN_TEST(test_fault_config_default_sensor_range);
    RUN_TEST(test_fault_config_default_can_timeout);
    RUN_TEST(test_fault_config_default_ts_lost);

    return UNITY_END();
}
