#include "test_usb_cmd.h"
#include "unity.h"
#include "usb_cmd.h"
#include "usbd_cdc_if.h"
#include "fsm_task.h"
#include "vcu_io.h"
#include "input_control.h"
#include <string.h>
#include <stdbool.h>


// ===========================================================================
// Tests
// ===========================================================================

void test_usb_cmd_echo_roundtrip(void) {
    // Send: [CMD_ECHO(0x45), LEN(4), 'p','i','n','g']
    uint8_t cmd_frame[] = {0x45, 0x04, 'p', 'i', 'n', 'g'};
    usb_cmd_rx(cmd_frame, sizeof(cmd_frame));
    // Test passes if no crash/hang during processing
}

void test_usb_cmd_request_state(void) {
    // Send: [CMD_REQUEST_STATE(0x04), LEN(0)]
    uint8_t cmd_frame[] = {0x04, 0x00};
    usb_cmd_rx(cmd_frame, sizeof(cmd_frame));
    // Test passes if command processes without crash
}

void test_usb_cmd_request_outputs(void) {
    // Send: [CMD_REQUEST_OUTPUTS(0x03), LEN(0)]
    uint8_t cmd_frame[] = {0x03, 0x00};
    usb_cmd_rx(cmd_frame, sizeof(cmd_frame));
    // Test passes if command processes without crash
}

void test_usb_cmd_spoof_set(void) {
    // Build a VcuInputs struct with some test values
    VcuInputs inputs = {
        .fault_flags = 0,
        .throttle_request = 0.5f,
        .brake_pressed = true,
        .rtd_button = false,
        .fwrd_switch = true,
        .rvrs_switch = false,
        .ts_active = true,
    };

    // Manually pack and send command
    uint8_t cmd_frame[2 + sizeof(VcuInputs)];
    cmd_frame[0] = 0x01;  // CMD_SPOOF_SET
    cmd_frame[1] = sizeof(VcuInputs);
    memcpy(&cmd_frame[2], &inputs, sizeof(VcuInputs));

    usb_cmd_rx(cmd_frame, sizeof(cmd_frame));

    // Verify the spoofed inputs were applied via public API
    VcuInputs current;
    vcu_gather_inputs(&current);
    TEST_ASSERT_TRUE(current.fwrd_switch);
    TEST_ASSERT_TRUE(current.ts_active);
    TEST_ASSERT_TRUE(current.brake_pressed);
}

void test_usb_cmd_step(void) {
    // Send: [CMD_STEP(0x05), LEN(0)]
    uint8_t cmd_frame[] = {0x05, 0x00};

    usb_cmd_rx(cmd_frame, sizeof(cmd_frame));

    // STEP command should execute without error
    // (actual FSM state change depends on FSM task scheduling)
}

void test_usb_cmd_reset(void) {
    // Send: [CMD_RESET(0x07), LEN(0)]
    uint8_t cmd_frame[] = {0x07, 0x00};

    usb_cmd_rx(cmd_frame, sizeof(cmd_frame));

    // RESET command should execute without error
    // (actual FSM state reset depends on FSM task scheduling)
}
