# VCU Testing Strategy

## Overview
Testing covers two layers: unit tests (logic on host machine) and hardware tests (on actual STM32 device). Both are integrated into CI/CD pipeline to ensure code quality and safety.

## Requirements

1. **Hardware-agnostic FSM testing**
   - FSM logic must be unit testable without hardware dependencies
   - Input injection via mock functions simulates sensor data
   - Enables deterministic testing of all state transitions
   - No flaky tests due to real-time behavior

2. **Fault detection validation**
   - All fault conditions must be testable (APPS disagree, implausibility, range errors)
   - Fault responses must match configured policies
   - Must validate BSPD (brake+throttle) software check per FSAE rules
   - Must verify inverter heartbeat timeout behavior

3. **Safety-critical code coverage**
   - FSM state transitions: all events and state combinations
   - Sensor fault detection: comprehensive edge cases
   - Motor torque commands: enable/disable lockout, direction changes
   - Safe defaults: verify all outputs de-energized on boot

4. **Hardware-in-the-loop (HIL) validation**
   - Real CAN communication with inverter simulator
   - GPIO relay outputs toggled and verified
   - ADC sensor acquisition tested with known inputs
   - On-device tests included in firmware build

5. **Continuous Integration with CI/CD**
   - Automated build on every commit
   - Unit tests run on GitHub Actions (host environment)
   - Test results reported and archived
   - Firmware binary generated for flash/deployment

## Unit Tests

Host-based tests validate hardware-agnostic logic without requiring STM32 hardware.

**Framework**: Unity (C unit test library)

**Location**: `Core/Tests/unit/`

**Files**:
- `test_fsm.c` - FSM state transitions, state machine logic
- `test_internals.c` - Sensor processing, fault detection
- `stubs.c` - Mock implementations of HAL calls (CAN, GPIO, ADC)

**Running**:
```bash
cd Core/Tests/unit
cmake -B build  # first time only
cmake --build build
./build/test_runner
```

**Adding Tests**:
1. Write test function: `void test_function_name(void) { /* assertions */ }`
2. Add source file to `CMakeLists.txt`
3. Register test in `runner.c`: `RUN_TEST(test_function_name)`
4. Run `cmake --build build` to recompile

**Test Coverage**:
- FSM state transitions (all events, state combinations)
- Sensor fault detection (APPS disagreement, plausibility)
- Motor torque command computation
- Fault response policies
- Safe defaults during boot

## Hardware Tests

On-device tests run on actual STM32 hardware, validating real peripherals.

**Location**: `Core/Tests/hardware/`

**Files**:
- `test_board_outputs.c` - GPIO relay control
- `test_can.c` - CAN bus communication
- `test_fsm_hil.c` - HIL (Hardware-in-the-Loop) FSM validation
- `test_motor_controller.c` - Inverter CAN communication
- `fsm_test_helpers.c/.h` - Utility functions for injecting test inputs
- `hardware_test_runner.c/.h` - Test framework for on-device execution

**Input Injection:** `vcu_spoof_inputs()`

Hardware tests inject simulated sensor inputs via `vcu_spoof_inputs()`:
```c
VcuInputs test_inputs = {
    .throttle = 0.5f,              // 50% pedal
    .brake_pressed = true,
    .fwrd_switch = true,
    .fault_flags = 0u,             // No faults
};
vcu_spoof_inputs(&test_inputs);    // FSM uses these instead of real sensors
```

Enables deterministic testing without external hardware.

**Running on Hardware**:
- Included in main firmware build (optional via build flag)
- Tests run during boot via `hardware_test_pre_boot()`
- Results logged to USB COM port and available for inspection
- Development tool only (disabled in production builds)
	- todo: reconsider, perhaps split development tests, and POST boot tests. I think it makes sense for some tests to run *every* time the car starts.

## CI/CD Integration

**GitHub Actions** (`.github/workflows/build.yml`):

1. **Firmware Build Job**
   - Installs: cmake, ninja, gcc-arm-none-eabi
   - Compiles: Debug and Release targets
   - Artifact: `build/Debug/DMS-24-RTOS-VCU.elf`

2. **Unit Tests Job**
   - Installs: cmake, gcc (native)
   - Builds test suite: `cmake -S Core/Tests/unit -B Core/Tests/unit/build`
   - Runs: `ctest --test-dir Core/Tests/unit/build --output-on-failure`
   - Uploads Unity results as artifact

3. **Hardware Unit Test Job**
   - Section reserved for future implementation
   - Installs: cmake, STM32 toolchain to team hosted runner
   - Flash's: STM32F407 DISC1 Development board (without VCU peripherals)
   - Uploads Unity results as artifact, via USB output

**Triggers**: Push to any branch, pull requests

**Test Artifacts**: GitHub Actions stores:
- Build logs
- Test result XML (for GitHub UI integration)
- Compiled binaries for local testing

## Pre-boot Hardware Tests

Optional validation runs automatically during boot:

1. GPIO outputs tested (relay control)
2. CAN bus tested (frame transmission)
3. Sensor ADC tested (channel connectivity)
4. Results captured in `s_pre_boot_result`
5. If failures detected → Error_Handler()

## References

- **Code**: `Core/Tests/unit/`, `Core/Tests/hardware/`, `.github/workflows/build.yml`
- **FSAE Rules**: Safety validation requirements (EV4.7 BSPD, EV8.1.6 CAN watchdog demonstrations)
- **Related Docs**: `unit-testing.md`, `vcu-finite-state-machine.md`, `vcu-sensors.md`, `vcu-fault-handling.md`
