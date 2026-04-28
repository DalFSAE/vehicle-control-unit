#include "unity/unity.h"

void test_neutral_to_forward_requires_brake(void);
void test_forward_apps_disagree_cut_throttle_holds_state(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_neutral_to_forward_requires_brake);
    RUN_TEST(test_forward_apps_disagree_cut_throttle_holds_state);
    return UNITY_END();
}