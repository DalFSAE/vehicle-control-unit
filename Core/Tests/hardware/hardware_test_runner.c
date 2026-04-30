#include "hardware_test_runner.h"
#include "unity.h"
  
// ===========================================================================
// Hardware Test Runner
// Preforms suite of tests on vcu and vehicle, to very all subsystems are OK
// ===========================================================================


void setUp(void) {
    /* runs before each test */
}

void tearDown(void) {
    /* runs after each test */
}

void test_add_returns_correct_sum(void) {
    TEST_ASSERT_EQUAL_INT(5, (2+3));
}


int hardware_test_runner(void) {
    UNITY_BEGIN();
    RUN_TEST(test_add_returns_correct_sum);
    return UNITY_END();
}

