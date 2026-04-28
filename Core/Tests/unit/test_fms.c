#include "unity/unity.h"
#include "fsm.h"
#include "vcu_io.h"

void test_neutral_to_forward_requires_brake(void) {
    VcuInputs in = { .fwrd_switch = true, .rtd_button = true,
                     .brake_pressed = false, .ts_active = true };
    VcuOutputs out = {0};
    FsmFaultConfig_t cfg = FaultConfig_default();
    FsmState_t next = step_fsm(ST_NEUTRAL, &cfg, &in, &out);
    TEST_ASSERT_EQUAL(ST_NEUTRAL, next);
    TEST_ASSERT_FALSE(out.throttle_enabled);
}

void test_forward_apps_disagree_cut_throttle_holds_state(void) {
    VcuInputs in = { .fwrd_switch = true, .ts_active = true,
                     .fault_flags = FAULT_APPS_DISAGREE };
    VcuOutputs out = {0};
    FsmFaultConfig_t cfg = FaultConfig_default();
    cfg.apps_disagree = FAULT_RESP_CUT_THROTTLE;
    FsmState_t next = step_fsm(ST_FORWARD, &cfg, &in, &out);
    TEST_ASSERT_EQUAL(ST_FORWARD, next);
    TEST_ASSERT_FALSE(out.throttle_enabled);
}