#include "unity.h"
#include "board_outputs.h"

void test_board_outputs_default_state(void) {
    for (int i = 0; i < (int)OUTPUT_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT32(0u, board_output_get_state((OutputChannel_t)i));
    }
}

void test_output_enable_disable(void) {
    board_output_enable(OUTPUT_DEBUG_LED4);
    TEST_ASSERT_EQUAL_UINT32(1u, board_output_get_state(OUTPUT_DEBUG_LED4));
    board_output_disable(OUTPUT_DEBUG_LED4);
    TEST_ASSERT_EQUAL_UINT32(0u, board_output_get_state(OUTPUT_DEBUG_LED4));
}
