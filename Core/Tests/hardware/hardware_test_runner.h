#pragma once
#include <stdint.h>

// Run before the OS scheduler starts. No osDelay allowed.
// Returns Unity failure count (0 = all passed).
uint32_t hardware_test_pre_boot(void);

// Run after the OS scheduler starts, from a task context.
// Returns Unity failure count (0 = all passed).
uint32_t hardware_test_post_boot(void);
