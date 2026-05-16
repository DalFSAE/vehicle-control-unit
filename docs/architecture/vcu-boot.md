# VCU Boot Sequence

## Overview
Boot sequence initializes hardware, validates system health, and starts FreeRTOS tasks. The process is split into pre-boot (hardware checks before scheduler), post-boot (after scheduler), and task startup.

## Requirements

1. **Hardware-agnostic control logic**
   - Boot sequence must be testable via injected inputs
   - FSM must validate during boot via hardware tests
   - Input signals must be normalized before FSM processing

2. **Safe initialization order**
   - All outputs must be de-energized on power-up (safe default)
   - Inverter disabled until FSM ready to control it
   - CAN watchdog must be active before any external system communication

3. **Fault detection during boot**
   - Pre-boot tests validate GPIO, CAN, sensor connectivity
   - Boot failure must halt vehicle operation (Error_Handler)
   - Results must be logged for diagnostics

4. **Real-time task scheduling**
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

**Post-boot 
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
| FSM           | 512×4 bytes | Normal      | Vehicle state control (10ms period) |
| CAN           | 512×4 bytes | AboveNormal | CAN TX/RX handling (5ms period)     |
| Sensor Input  | 512×4 bytes | AboveNormal | ADC + fault detection (~10ms)       |
| Heartbeat     | 256×4 bytes | Low         | System watchdog tick (250ms)        |
| Logger        | 256×4 bytes | Normal      | USB telemetry output                |
| Hardware Test | 512×4 bytes | Low         | Optional on-device validation       |
todo: determine if the current priority is okay. I recall there being a possible bug where a high priority task would get stuck waiting for a mutex. Resolve and open issue before merging.
todo: review current list of tasks, determine if when a task should be made vs including on the main FSM thread.
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
