# VCU Logging and Telemetry

## Overview
Logging system provides two channels: event-based human-readable logs and periodic structured telemetry. Both flow through USB CDC for real-time visibility during development and diagnostics.

## Requirements

1. **Event logging for diagnostics**
   - Human-readable log messages with timestamps
   - Low-frequency, high-information-density events
   - Traceability for fault analysis and system verification
   - Limited availability before logging subsystem fully initialized; `LOG_EVENT()` buffers early events automatically (pre-init buffer)

2. **Telemetry for monitoring**
   - Periodic fixed-format sensor data for real-time monitoring
   - Deterministic structure for external logging tools
   - Transmit via CAN0 (see `can.md`)
   - Enable tracking of vehicle behavior during testing

3. **USB CDC output during development**
   - Real-time stream accessible via USB CDC
   - Allows debugging without removing vehicle from test stand
   - Serial format: 115200 baud, 8N1
   - Must not block control loop (asynchronous logging)

4. **Early event capture**
   - System must log events that occur before logging subsystem ready
   - Pre-init buffer captures up to 64 lines × 240 chars
   - Events flushed to USB after log_init() completes
   - Prevents loss of boot/initialization diagnostics

5. **Task-safe queue for multi-threaded logging**
   - FSM, sensor, and CAN tasks generate log events
   - Logging cannot block realtime tasks
   - Queue-based architecture with dedicated logger task

## Logging Architecture

**Event Logging** - Human-readable log messages
- Low frequency, high information density
- Examples: "APPS mismatch fault latched", "Inverter enable denied: IMD fault active"
- Used for state changes, fault events, mode transitions
- Timestamps via HAL_GetTick()

**Telemetry** - Periodic fixed-format sensor data
- High frequency, deterministic structure
- Examples: "APPS1=1.82V", "Brake=420psi", "FSM_state=DRIVE_READY"
- CAN messages transmitted periodically
- Enables real-time monitoring and logging to external tools

## Log Interface

**Log Events** - Macro `LOG_EVENT()`

```c
LOG_EVENT(LOG_LEVEL_INFO, EVT_BOOT, test_count, failure_count);
LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, FAULT_APPS_DISAGREE, response_policy);
```

Signature: `LOG_EVENT(level, event_type, arg0, arg1)`

**Log Formatted** - Macro `LOG_PRINTF()`

```c
LOG_PRINTF("Sensor value: %d counts\r\n", adc_value);
```

Printf-style string formatting for flexible messaging.

## Output Channels

**USB CDC (Serial)** - Primary during development
- Connected via ST-LINK debugger (virtual COM port)
- 115200 baud, 8N1
- Real-time stream of events + telemetry
- Can be captured to file for post-analysis

**CAN Bus** - Periodic telemetry only
- Frame ID: configured per message type (see `can0_powertrain.c`)
- Contains sensor values, FSM state, relay status
- Monitored by dashboard and data logging systems
- Deterministic, suitable for high-speed acquisition

## Pre-init Buffering

Logging initializes late (after 500ms scheduler delay) to avoid USB enumeration issues. Early logs are buffered:

- Pre-init buffer: 64 lines × 240 chars each
- Captures boot messages before logging subsystem ready
- Flushed to USB when log_init() completes
- Ensures no early events are lost

See `log.c` for implementation.

## Log Levels

| Level | Purpose | Usage |
|-------|---------|-------|
| ERROR | Critical failures | Faults, watchdog violations |
| WARN | Non-critical issues | Sensor out of nominal range |
| INFO | Normal operation | State changes, mode transitions |
| DEBUG | Development visibility | ADC values, timing info |

Compile-time filtering can suppress low-priority logs to reduce bandwidth.

## Telemetry Publishing

CAN telemetry messages published periodically:

**Sensor Data**
- APPS1/APPS2 voltages
- Brake pressures
- Wheel speeds
- Motor temperature

**FSM State**
- Current state (ENTRY, STANDBY, NEUTRAL, FORWARD, REVERSE)
- Active faults
- Mode switches

**Relay Status**
- Bitmap of energized relays
- Watchdog tick counter

See `can0_powertrain.c` for CAN message formats and encoding.

## Integration with FSM

FSM logs every state transition:
```c
LOG_EVENT(LOG_LEVEL_INFO, EVT_STATE_CHANGE, prev_state, new_state);
```

Faults logged when set:
```c
LOG_EVENT(LOG_LEVEL_ERROR, EVT_FAULT_SET, fault_flag, response_policy);
```

Enables debugging of state machine logic and fault responses.

## USB Logger Task

FreeRTOS task manages USB output:
- Dequeues log messages from task-safe queue
- Formats and transmits via CDC
- Runs at normal priority (does not block FSM or CAN)
- Pre-init messages flushed after initialization

See `log.c` for task implementation.

## References

- **Code**: `Core/User/log.c`, `Core/User/log.h`, `Core/User/dash.c`
- **FSAE Rules**: General diagnostics/safety logging not explicitly required but recommended
- **Related Docs**: `vcu-boot.md`, `vcu-finite-state-machine.md`, `can.md`, `logging-and-telemetry.md`
