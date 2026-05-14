#pragma once
#include <stdint.h>

typedef struct {
    uint16_t tests_run;
    uint16_t failures;
    uint16_t ignored;
} BootResult_t;

BootResult_t make_result(void);

// Run before the OS scheduler starts. No osDelay allowed.
BootResult_t hardware_test_pre_boot(void);

// Run after the OS scheduler starts, but before task creation. osDelay allowed.
BootResult_t hardware_test_post_boot(void);

// FreeRTOS task entry point. Runs the post-boot suite and terminates.
void hardware_post_test_task(void *argument);
