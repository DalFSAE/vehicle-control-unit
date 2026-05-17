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

3. **Inverter status monitoring**
   - Inverter must transmit M170 status message ~50ms period
   - Loss of heartbeat within 200ms triggers torque cut
   - Inverter faults (overspeed, overtemp) reported via CAN
   - VCU responds based on fault severity

4. **Sensor fault detection at high frequency**
   - Sensor plausibility checks run every 10ms (FSM loop rate)
   - APPS disagreement, brake+throttle check, range validation
   - Immediate response to detected faults (no delayed reaction)

5. **CAN watchdog for safety shutdown system compliance**
   - Per FSAE EV8.1.6: vehicles using CAN as safety element must shutdown if CAN broken
   - Alternative: dedicated hardware watchdog with relay output
   - Demonstrates compliance at technical inspection

## FreeRTOS System Watchdog

**Heartbeat Task** (250ms period)
- Runs at low priority
- Increments counter that external hardware monitors
- If counter stops incrementing → hardware triggers system reset

**Purpose**: Detect if FreeRTOS scheduler hangs (deadlock, infinite loop)

**Implementation**: `app.c`, `app_heartbeat_task()`

If scheduler freezes, heartbeat task never runs, counter stops, watchdog resets MCU.

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

**BMS Heartbeat** (Can be configured)
- Battery management system sends status periodically
- Timeout indicates loss of battery communication
- Vehicle shuts down safely (no operation possible without battery)

**Dashboard Heartbeat** (Optional)
- Telemetry receiver should acknowledge receipt
- Can detect if data link to external logging is broken

## Inverter Fault Monitoring

Inverter reports internal faults via M170 CAN message:

- `inv_vsm_state` - inverter state machine state
- `post_fault` - faults detected after last clear (latched)
- `run_fault` - faults detected while running (transient)

Examples:
- Overspeed: Motor exceeds programmed limit
- Over-temperature: Winding or power stage too hot
- Over-voltage: Battery voltage spike detected
- Under-voltage: Battery voltage dipped below limit
- CAN timeout: Inverter lost command stream for >500ms

VCU responds based on fault severity:
- Non-critical faults: Log and continue operation
- Critical faults: Cut torque, return to NEUTRAL
- Faults shall clear when the the power to the inverter is cycled

## Watchdog Timing

| Watchdog | Period | Timeout | Response |
|----------|--------|---------|----------|
| FreeRTOS heartbeat | 250ms | Hardware dependent | System reset |
| Inverter CAN | ~50ms | 200ms | Torque cut, FSM fault |
| BMS CAN | ~100ms | 1000ms | Return to neutral |
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

## Safe Defaults

If all watchdogs fail:
- No command received from FSM → use last known command (conservative)
- Sensors unavailable → assume maximum safe mode (brakes applied, throttle cut)
- CAN unavailable → motor disabled, vehicle cannot drive

## FSAE EV8.1.6 Compliance (CAN Watchdog for Safety)

Rule EV8.1.6 states: "Vehicles that use CANbus as a fundamental element of the safety shutdown system will be required to document and demonstrate shutdown equivalent to BRB operation if CAN communication is interrupted."

**Current Implementation**:
- Software watchdog in VCU monitors CAN heartbeats
- On heartbeat loss → FSM triggers SDC_OPEN response
- Vehicle is immediately de-energized (equivalent to BRB - Brake Response Button)

**Demonstration at Technical Inspection**:
- Must show that loss of CAN communication triggers shutdown
- Verify no torque is produced when CAN inactive
- Demonstrate that vehicle cannot move if CAN is disconnected

**Alternative Approach** (not currently implemented):
- Dedicated hardware watchdog circuit with relay output
- Monitors CAN traffic independently
- If CAN silent for >1 second, relay opens SDC
- No dependency on VCU software

## Future Enhancements

Potential watchdog improvements:
- Dedicated hardware watchdog circuit (independent of MCU)
- Redundant CAN bus monitoring
- Accelerometer-based overspeed detection
- Temperature sensor monitoring (motor/inverter)
- Isolation of safety-critical shutdown path

## References

- **Code**: `Core/User/app.c`, `Core/User/motor_controller.c`, `Core/User/can_task.c`
- **FSAE Rules**: 
  - EV4.7 - Sensor fault detection (BSPD check)
  - EV8.1.6 - CAN watchdog for safety shutdown system
- **Related Docs**: `vcu-motor-control.md`, `vcu-sensors.md`, `vcu-finite-state-machine.md`, `vcu-fault-handling.md`
