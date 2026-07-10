#include "test_motor_controller.h"
#include "fsm_test_helpers.h"
#include "motor_controller.h"
#include "can_bus.h"
#include "can0_powertrain.h"
#include "can_task.h"
#include "hardware_test_runner.h"
#include "fsm_task.h"
#include "unity.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include "vcu_io.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

extern CAN_HandleTypeDef hcan1;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool wait_for_mc_alive(uint32_t timeout_ms) {
    uint32_t deadline = HAL_GetTick() + timeout_ms;
    while (HAL_GetTick() < deadline) {
        if (!mc_has_timeout())
            return true;
        osDelay(20);
    }
    return false;
}

static volatile bool    s_rx_fired = false;
static volatile uint8_t s_rx_data[8];
static volatile uint8_t s_rx_len   = 0;

static void capture_rx(uint32_t id, const uint8_t *data, size_t len) {
    (void)id;
    s_rx_len = (uint8_t)(len > 8 ? 8 : len);
    memcpy((void *)s_rx_data, data, s_rx_len);
    s_rx_fired = true;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// No inverter required. Verifies the mutex-protected command cache preserves
// all fields through a set/get round-trip.
void test_mc_cmd_cache_roundtrip(void) {
    const MotorControllerCmd_t tx = {
        .inv_enable              = true,
        .inv_discharge           = false,
        .speed_mode_enable       = false,
        .motor_direction_forward = true,
        .torque_command_nm       = 42.5f,
        .torque_limit_nm         = MC_TORQUE_LIMIT_NM,
        .speed_command_rpm       = 1234,
        .rolling_counter         = 7,
    };
    motor_controller_set_cmd(&tx);

    MotorControllerCmd_t rx = {0};
    motor_controller_get_cmd(&rx);

    TEST_ASSERT_EQUAL(tx.inv_enable,              rx.inv_enable);
    TEST_ASSERT_EQUAL(tx.inv_discharge,           rx.inv_discharge);
    TEST_ASSERT_EQUAL(tx.speed_mode_enable,       rx.speed_mode_enable);
    TEST_ASSERT_EQUAL(tx.motor_direction_forward, rx.motor_direction_forward);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, tx.torque_command_nm, rx.torque_command_nm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, tx.torque_limit_nm,   rx.torque_limit_nm);
    TEST_ASSERT_EQUAL(tx.speed_command_rpm,       rx.speed_command_rpm);
    TEST_ASSERT_EQUAL(tx.rolling_counter,         rx.rolling_counter);
}

// No inverter required. Uses CAN loopback to verify the M192 frame byte
// layout against the PM100DX CAN protocol spec (section 2.2).
//
//   Bytes 0-1  Torque command  little-endian, 0.1 Nm/count (10 Nm -> 100)
//   Bytes 2-3  Speed command   little-endian
//   Byte  4    Direction       0 = Reverse, 1 = Forward
//   Byte  5    Control bits    [0]=Enable [1]=Discharge [2]=SpeedMode
//   Bytes 6-7  Torque limit    little-endian, 0.1 Nm/count (0 = use EEPROM)
void test_mc_cmd_encoding_loopback(void) {
    osThreadSuspend(can_task_get_handle());

    can_bus_init(&hcan1, CAN_MODE_LOOPBACK);
    can_bus_set_rx_hook(capture_rx);
    s_rx_fired = false;

    const MotorControllerCmd_t cmd = {
        .inv_enable              = true,
        .inv_discharge           = false,
        .speed_mode_enable       = false,
        .motor_direction_forward = true,
        .torque_command_nm       = 10.0f,
        .torque_limit_nm         = 0.0f,
        .speed_command_rpm       = 0,
        .rolling_counter         = 0,
    };
    can_tx_send_inverter_cmd(&cmd);
    osDelay(20);

    TEST_ASSERT_TRUE_MESSAGE(s_rx_fired, "No frame captured in loopback");
    TEST_ASSERT_EQUAL_UINT8(CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_LENGTH, s_rx_len);

    // Bytes 0-1: 10 Nm at 0.1 Nm/count = 100 counts
    uint16_t torque_raw = (uint16_t)s_rx_data[0] | ((uint16_t)s_rx_data[1] << 8);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(100u, torque_raw, "Torque encoding mismatch");

    // Byte 4: 1 = Forward per PM100DX spec
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1u, s_rx_data[4], "Direction byte: expected 1 (Forward)");

    // Byte 5: bit[0]=Enable, bit[1]=Discharge, bit[2]=SpeedMode
    TEST_ASSERT_TRUE_MESSAGE( s_rx_data[5] & 0x01u, "Inverter enable bit not set");
    TEST_ASSERT_FALSE_MESSAGE(s_rx_data[5] & 0x02u, "Discharge bit should be clear");
    TEST_ASSERT_FALSE_MESSAGE(s_rx_data[5] & 0x04u, "Speed mode bit should be clear");

    // Bytes 6-7: 0 = use EEPROM torque limit
    uint16_t limit_raw = (uint16_t)s_rx_data[6] | ((uint16_t)s_rx_data[7] << 8);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0u, limit_raw, "Torque limit should be 0 (EEPROM default)");

    can_bus_set_rx_hook(NULL);
    can_bus_init(&hcan1, CAN_MODE_NORMAL);
    osThreadResume(can_task_get_handle());
}

// Requires inverter on bus.
void test_mc_heartbeat_received(void) {
    if (!wait_for_mc_alive(MC_HEARTBEAT_TIMEOUT_MS * 3u)) {
        TEST_IGNORE_MESSAGE("requires inverter on bus");
    }
    TEST_ASSERT_FALSE_MESSAGE(mc_has_timeout(), "Inverter M170 heartbeat timed out");
}

// Requires inverter on bus. Gives the inverter up to 5 s to complete its VSM
// startup sequence (precharge -> ready) before asserting.
void test_mc_vsm_is_ready(void) {
    if (!wait_for_mc_alive(MC_HEARTBEAT_TIMEOUT_MS * 3u)) {
        TEST_IGNORE_MESSAGE("requires inverter on bus");
    }
    uint32_t deadline = HAL_GetTick() + 5000u;
    while (HAL_GetTick() < deadline) {
        if (mc_is_ready())
            break;
        osDelay(50);
    }
    TEST_ASSERT_TRUE_MESSAGE(mc_is_ready(), "Inverter did not reach VSM READY (state >= 5) within 5 s");
}

// Requires inverter on bus.
void test_mc_no_active_faults(void) {
    if (!wait_for_mc_alive(MC_HEARTBEAT_TIMEOUT_MS * 3u)) {
        TEST_IGNORE_MESSAGE("requires inverter on bus");
    }
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0u, mc_fault_bitmap(), "Inverter reported active faults");
}

// Sweeps throttle from 0 to 100% in FORWARD state and verifies the motor
// controller command cache at each step. fsm_task emits EVT_IO_CHANGE at
// DEBUG level automatically on each output change, producing a readable
// trace of the full sweep on serial.
void test_mc_throttle_sweep(void) {
    suspend_sensor();
    walk_to_neutral();
    walk_to_forward();
    TEST_ASSERT_EQUAL_MESSAGE(ST_FORWARD, g_fsm_state, "FSM must reach FORWARD before sweep");

    static const float steps[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); i++) {
        g_spoof.throttle_request = steps[i];
        vcu_spoof_inputs(&g_spoof);
        osDelay(FSM_SETTLE_MS); // FSM ticks, applies outputs, emits EVT_IO_CHANGE

        MotorControllerCmd_t cmd;
        motor_controller_get_cmd(&cmd);

        float expected_nm = steps[i] * MC_TORQUE_MAX_NM;
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5f, expected_nm, cmd.torque_command_nm,
            "Torque command does not match throttle request");
        TEST_ASSERT_TRUE_MESSAGE(cmd.inv_enable,              "Inverter must be enabled in FORWARD");
        TEST_ASSERT_TRUE_MESSAGE(cmd.motor_direction_forward, "Direction must be forward");
    }

    clear_inputs();
    resume_sensor();
}

BootResult_t run_mc_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mc_cmd_cache_roundtrip);
    RUN_TEST(test_mc_cmd_encoding_loopback);
    RUN_TEST(test_mc_throttle_sweep);
    RUN_TEST(test_mc_heartbeat_received);
    RUN_TEST(test_mc_vsm_is_ready);
    RUN_TEST(test_mc_no_active_faults);
    UNITY_END();
    return make_result();
}
