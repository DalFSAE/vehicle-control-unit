#include "unity.h"
#include "stdbool.h"

// Unity boilerplate
void setUp(void) {}
void tearDown(void) {}

// ===========================================================================
// Tests
// ===========================================================================


void test_throw_fault (void) {
    TEST_ASSERT_TRUE(false); // this is to test CI/CD, should normally be true
}

// ===========================================================================
// Entry point
// ===========================================================================

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_check_platform);
    return UNITY_END();
}
