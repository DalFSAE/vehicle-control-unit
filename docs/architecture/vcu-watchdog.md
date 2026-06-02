# VCU Watchdog Monitoring

## Overview
Watchdog system monitors health of critical subsystems. Multiple watchdog mechanisms (FreeRTOS tick, CAN heartbeats, inverter status) ensure vehicle responds immediately to failures instead of hanging in unsafe state.

## Requirements

1. **System deadlock detection**
   - FreeRTOS scheduler must remain responsive (no infinite loops, no deadlocks)
   - Independent watchdog monitors MCU tick counter
   - If counter stops, hardware triggers system reset
   - Prevents hung vehicle from remaining powered (safety critical)

2. **CAN communication heartbeat monitoring (FSAE EV8.1.6)**
   - Critical systems must transmit periodic CAN messages
   - Loss of any required heartbeat triggers safe shutdown
   - Timeout thresholds must be conservative (allow for message jitter)
   - Integration with fault handling system required
   - See also [vcu-shutdown-circuit.md](vcu-shutdown-circuit.md)

3. **Inverter status monitoring**
   - Inverter must transmit M170 status message ~50ms period
   - Loss of heartbeat within 200ms triggers torque cut
   - Inverter faults (overspeed, overtemp) reported via CAN
   - VCU responds based on fault severity
   - See also [vcu-motor-control.md](vcu-motor-control.md)

4. **Sensor fault detection at high frequency**
   - PPC checks run at sensor task rate
   - Immediate response to detected faults (no delayed reaction)
   - See [vcu-fault-handling.md](vcu-fault-handling.md) for the full PPC definition

5. **CAN watchdog for safety shutdown system compliance**
   - Per FSAE EV8.1.6: vehicles using CAN as safety element must shutdown if CAN broken
   - See [vcu-shutdown-circuit.md](vcu-shutdown-circuit.md) and [can.md](../can.md)

## FreeRTOS System Watchdog

**Heartbeat Task** (250ms period)
- Runs at low priority
- Increments counter that external hardware monitors
- If counter stops incrementing → hardware triggers system reset

**Purpose**: Detect if FreeRTOS scheduler hangs (deadlock, infinite loop)

See `app.c`, `app_heartbeat_task()`.

## CAN Heartbeat Monitoring

Critical subsystems must transmit periodic CAN messages or vehicle enters fault state.

**Inverter Heartbeat** (Expected: ~50ms, Timeout: 200ms)
- Inverter transmits M170 Internal States message
- VCU tracks reception via `s_inv.last_rx_tick_ms`
- If no message received within 200ms:
  - Log fault: `FAULT_CAN_TIMEOUT`
  - Torque immediately cut
  - FSM transitions to fault state (typically returns to NEUTRAL)
  - Inverter enable must be re-established

See `motor_controller.c` for heartbeat tracking logic.

**HVC Heartbeat** (includes BMS status)
- High Voltage Controller sends status periodically, including BMS data
- Timeout indicates loss of HV system communication
- Vehicle shuts down safely (no operation possible without HV)

**Dashboard Heartbeat** (Optional)
- Telemetry receiver should acknowledge receipt
- Can detect if data link to external logging is broken

## Inverter Fault Monitoring

See [vcu-motor-control.md](vcu-motor-control.md) for the full list of inverter faults and message details.

VCU responds based on fault severity:
- Non-critical faults: log and continue operation
- Critical faults: cut torque, return to NEUTRAL
- Faults clear when inverter power is cycled

## Watchdog Timing

| Watchdog | Period | Timeout | Response |
|----------|--------|---------|----------|
| FreeRTOS heartbeat | 250ms | Hardware dependent | System reset |
| Inverter CAN | ~50ms | 200ms | Torque cut, FSM fault |
| HVC CAN | ~100ms | 1000ms | Return to neutral |
| Sensor check | ~10ms | Immediate | Fault raised |

Timing ensures:
- Inverter fault detected within 200ms (safe response time)
- Sensor faults detected within 10ms (loop frequency)
- System watchdog prevents indefinite hangs

## Integration with FSM

FSM consumes fault signals from watchdog monitoring:

1. Sensor/CAN watchdog sets fault flag in `VcuInputs`
2. FSM state machine checks fault flags
3. If fault active → execute fault response policy
4. Log fault event with context

Example flow:
```
Inverter heartbeat timeout
  → motor_controller detects missed heartbeat
  → Sets FAULT_CAN_TIMEOUT flag
  → FSM state machine sees fault
  → Executes fault response (typically RETURN_NEUTRAL)
  → Vehicle safely stops
```

## FSAE EV8.1.6 Compliance (CAN Watchdog for Safety)

On CAN heartbeat loss, the VCU de-energizes `SDC_RELAY`, opening the shutdown circuit. See [vcu-shutdown-circuit.md](vcu-shutdown-circuit.md) for the full compliance details and technical inspection requirements.

## References

- **Code**: `Core/User/app.c`, `Core/User/motor_controller.c`, `Core/User/can_task.c`
- **FSAE Rules**:
  - EV.7.7 - BSPD hardware circuit
  - EV8.1.6 - CAN watchdog for safety shutdown system
- **Related Docs**: `vcu-motor-control.md`, `vcu-sensors.md`, `vcu-finite-state-machine.md`, `vcu-fault-handling.md`
