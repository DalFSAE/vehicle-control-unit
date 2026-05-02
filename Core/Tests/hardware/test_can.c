#include "test_can.h"
#include "unity.h"
#include "can_bus.h"
#include "can0_powertrain.h"
#include "motor_controller.h"
#include "dash.h"
#include "can_task.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

extern CAN_HandleTypeDef hcan1;

// ---------------------------------------------------------------------------
// Loopback capture state written from ISR via rx hook.
// ---------------------------------------------------------------------------

static volatile bool     s_rx_fired = false;
static volatile uint32_t s_rx_id = 0;
static volatile uint8_t  s_rx_data[8];
static volatile uint8_t  s_rx_len = 0;

static void capture_rx(uint32_t id, const uint8_t *data, size_t len) {
    s_rx_id = id;
    s_rx_len = (uint8_t)(len > 8 ? 8 : len);
    memcpy((void *)s_rx_data, data, s_rx_len);
    s_rx_fired = true;
}

// ---------------------------------------------------------------------------
// test_can_loopback
//
// Suspends can_task so only our explicit frame is on the bus, reinitialises
// CAN1 in loopback mode, fires a known M192 frame, and verifies the frame
// is echoed back through the RX FIFO. Restores normal mode afterwards.
// ---------------------------------------------------------------------------

void test_can_loopback(void) {
    // Stop can_task so its periodic M192 frames don't race with ours.
    osThreadSuspend(can_task_get_handle());

    can_bus_init(&hcan1, CAN_MODE_LOOPBACK);
    can_bus_set_rx_hook(capture_rx);
    s_rx_fired = false;

    // Rolling counter is a 4-bit field (bits [7:4] of byte 5, range 0-15).
    // Use a value within range so the pack/unpack round-trip is lossless.
    static const uint8_t TEST_ROLLING_CTR = 0x0B;

    MotorControllerCmd_t cmd = {
        .inv_enable = true,
        .motor_direction_forward = true,
        .torque_command_nm = 50.0f,
        .torque_limit_nm = MC_TORQUE_LIMIT_NM,
        .speed_command_rpm = 0,
        .rolling_counter = TEST_ROLLING_CTR,
    };
    can_tx_send_inverter_cmd(&cmd);

    osDelay(20); // allow ISR to fire

    TEST_ASSERT_TRUE_MESSAGE(s_rx_fired, "No RX frame captured in CAN loopback mode");
    TEST_ASSERT_EQUAL_UINT32(CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_FRAME_ID, s_rx_id);
    TEST_ASSERT_EQUAL_UINT8(CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_LENGTH, s_rx_len);

    // Decode and spot-check rolling counter and enable bit.
    struct can0_powertrain_m192_command_message_t decoded;
    int rc = can0_powertrain_m192_command_message_unpack(&decoded, (const uint8_t *)s_rx_data, s_rx_len);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT8(TEST_ROLLING_CTR, decoded.vcu_inv_rolling_counter);
    TEST_ASSERT_EQUAL_UINT8(1, decoded.vcu_inv_inverter_enable);

    // Restore normal mode and resume periodic TX.
    can_bus_set_rx_hook(NULL);
    can_bus_init(&hcan1, CAN_MODE_NORMAL);
    osThreadResume(can_task_get_handle());
}

void test_can_dash_led_msg(void) {
    osThreadSuspend(can_task_get_handle());

    can_bus_init(&hcan1, CAN_MODE_LOOPBACK);
    can_bus_set_rx_hook(capture_rx);
    s_rx_fired = false;

    DashLedCmd_t cmd = {
        .imd_ok = 1u,
        .bms_ok = 0u,
        .rtd    = 1u,
        .fault  = 0u,
    };
    dash_set_leds(&cmd);
    dash_tx_cmd();

    osDelay(20);

    TEST_ASSERT_TRUE_MESSAGE(s_rx_fired, "No RX frame captured for dash LED command");
    TEST_ASSERT_EQUAL_UINT32(CAN_ID_DASH_CMD, s_rx_id);
    TEST_ASSERT_EQUAL_UINT8(5, s_rx_len);
    TEST_ASSERT_EQUAL_UINT8(cmd.imd_ok, s_rx_data[0]);
    TEST_ASSERT_EQUAL_UINT8(cmd.bms_ok, s_rx_data[1]);
    TEST_ASSERT_EQUAL_UINT8(cmd.rtd,    s_rx_data[2]);
    TEST_ASSERT_EQUAL_UINT8(cmd.fault,  s_rx_data[3]);
    // byte 4 is led_test, managed by boot timer just verify it is 0 or 1
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(1u, s_rx_data[4]);

    can_bus_set_rx_hook(NULL);
    can_bus_init(&hcan1, CAN_MODE_NORMAL);
    osThreadResume(can_task_get_handle());
}

void test_if_inverter_alive(void) {
    TEST_IGNORE_MESSAGE("requires inverter on bus");
}

void test_if_bms_alive(void) {
    TEST_IGNORE_MESSAGE("requires BMS on bus");
}

BootResult_t run_can_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_can_loopback);
    RUN_TEST(test_can_dash_led_msg);
    RUN_TEST(test_if_inverter_alive);
    RUN_TEST(test_if_bms_alive);
    UNITY_END();
    return make_result();
}
