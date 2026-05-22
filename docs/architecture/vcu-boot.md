# VCU Boot Sequence

## Overview
Boot sequence initializes hardware, validates system health, and starts FreeRTOS tasks. The process is split into pre-boot (hardware checks before scheduler), post-boot (after scheduler), and task startup.

## Requirements

1. **Safe initialization order**
   - All outputs must be de-energized on power-up (safe default)
   - Inverter disabled until FSM ready to control it
   - CAN watchdog must be active before any external system communication (see [vcu-watchdog.md](vcu-watchdog.md))

2. **Fault detection during boot**
   - Pre-boot tests validate GPIO, CAN, and sensor connectivity before FSM starts (see [vcu-fault-handling.md](vcu-fault-handling.md))
     - **GPIO**: confirm relay functionality.
     - **CAN**: bus initialization returns no error; test in loopback mode
     - **Sensors**: ADC/DMA acquisition starts; sensor voltages within valid range
   - Boot failure must halt vehicle operation (Error_Handler)
   - Results must be logged for diagnostics

3. **Real-time task scheduling**
   - FSM task (10ms period) must run with priority to ensure control loop
   - Sensor input task (AboveNormal) must have responsive fault detection
   - CAN task (5ms) must prioritized for communication reliability

## Boot Stages

**Pre-boot (before scheduler)**
- Board outputs initialized (GPIO reset)
- Digital I/O subsystem initialized
- Buzzer initialized
- Hardware test pre-boot validation (optional)
- Error if pre-boot tests fail → Error_Handler()

**Post-boot (after scheduler)**
- 500ms scheduler delay for USB enumeration
- Logging system initialized (USB CDC)
- Boot event logged with test results

**Task startup**
- Sensor input task starts ADC/DMA acquisition
- FSM task begins vehicle state machine
- CAN task activates
- Logger task starts USB output
- Heartbeat task monitors system health

See `app.c` for implementation.

## FreeRTOS Task Configuration

| Task          | Stack Size  | Priority    | Purpose                             |
| ------------- | ----------- | ----------- | ----------------------------------- |
| FSM           | 512×4 bytes | Normal      | Vehicle state control               |
| CAN           | 512×4 bytes | AboveNormal | CAN TX/RX handling                  |
| Sensor Input  | 512×4 bytes | AboveNormal | ADC + fault detection               |
| Heartbeat     | 256×4 bytes | Low         | System watchdog tick                |
| Logger        | 256×4 bytes | Normal      | USB telemetry output                |
| Hardware Test | 512×4 bytes | Low         | Optional on-device validation       |

## Pre-boot Validation

Hardware tests validate system state before FSM starts:
- Runs `hardware_test_pre_boot()`
- Tests core components (GPIO, CAN, sensor readiness)
- Logs results to pre-init buffer
- Prevents FSM start if failures detected

Safe defaults are applied during boot:
- All relay outputs de-energized (GPIO_PIN_RESET)
- Throttle disabled
- Inverter disabled
- CAN watchdog active

## Integration with Logging

Boot events logged via `LOG_EVENT()`:
- Module: LOG_SRC_APP
- Event: EVT_BOOT
- Payload: test count, failure count

## References

- **Code**: `Core/User/app.c`, `Core/User/app.h`
- **FSAE Rules**: General power-up safety principles from EV rules
- **Related Docs**: `vcu-logging.md`, `vcu-testing.md`, `vcu-finite-state-machine.md`
