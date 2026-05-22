# VCU Fault Handling

## Overview
Fault handling system detects sensor/subsystem failures and executes safety responses. FSM implements a configurable fault policy that determines whether to cut throttle, return to neutral, trigger shutdown circuit, or latch the fault.

## Requirements

1. **FSAE EV4.7 Brake System Plausibility Device (BSPD)**
   - Must detect tractive system current + accelerator >25% simultaneous engagement

   **Software (VCU):**
   - Brake sensor AND accelerator >25% pedal → cut throttle
   - Motor shall stay cut until accelerator <5% pedal travel

   **Hardware (BSPD circuit):**
   - Should never trigger under normal operation; VCU software check must preempt it
   - If hardware BSPD triggers → open shutdown circuit
   - Shall be implemented in hardware and software
   - Shall be demonstrable at technical inspection

2. **Pedal Plausibility Checks (PPC)**
   - Two independent accelerator sensors (APPS1, APPS2) must agree
   - APPS1 + APPS2 voltages must sum to ~5V (±0.5V tolerance); deviation flags APPS_DISAGREE
   - Disagreement → APPS_DISAGREE fault with configurable response

3. **CAN heartbeat monitoring (FSAE EV8.1.6)**
   - Critical vehicle systems must maintain CAN heartbeats
   - Loss of inverter heartbeat (200ms) → return to neutral
   - Loss of BMS/HVC heartbeat → return to neutral or shutdown
   - Required for safety shutdown system compliance

4. **Configurable fault responses**
   - Not all faults require same action
   - Some faults warrant throttle cut, others require state change
   - Responses must be adjustable without code changes (optional)
   - Must support control of the shutdown circuit

5. **Fault logging and traceability**
   - Every fault must be logged with context
   - Every fault must send a notification to the driver via the dashboard display
   - Enables post-incident diagnostics and safety validation
   - Must integrate with telemetry system

## Fault Types

| Fault | Source | Detection | Default Response |
|-------|--------|-----------|-------------------|
| APPS_DISAGREE | Sensor input | Redundant pedal sensors disagree | CUT_THROTTLE |
| PEDAL_PLAUS | Sensor input | Brake + throttle simultaneously | RETURN_NEUTRAL |
| SENSOR_RANGE | Sensor input | Sensor value out of valid range | RETURN_NEUTRAL |
| CAN_TIMEOUT | Communication | Inverter or BMS heartbeat missing | RETURN_NEUTRAL |

See `sensor_control.c`, `motor_controller.c` for fault detection logic.

## Fault Response Policies

When a fault is detected, FSM executes a configured response:

**CUT_THROTTLE** (`FAULT_RESP_CUT_THROTTLE`)
- Immediately zero out torque command
- Keep FSM in current state (no state change)
- Vehicle maintains speed but cannot accelerate
- Used for isolated sensor disagreement

**RETURN_NEUTRAL** (`FAULT_RESP_RETURN_NEUTRAL`)
- Trigger FSM_EV_STOP event
- FSM returns to NEUTRAL state
- Throttle disabled, inverter disabled
- Used for critical safety failures (pedal plausibility)

**SDC_OPEN** (`FAULT_RESP_SDC_OPEN`)
- Trigger Shutdown Circuit (SDC relay open)
- Immediately cuts high-voltage power
- Most severe response (reserved for critical faults)

**LATCH_FAULT** (`FAULT_RESP_LATCH_FAULT`)
- Fault stays active until manual reset
- Prevents fault from clearing while vehicle running
- Used to ensure driver awareness of persistent issues
- Should only be called if the car is deemed *unsafe*

## Fault Configuration

Fault responses are configured via `FsmFaultConfig_t`:

```c
FsmFaultConfig_t cfg = {
    .apps_disagree   = FAULT_RESP_CUT_THROTTLE,
    .pedal_plaus     = FAULT_RESP_RETURN_NEUTRAL,
    .sensor_range    = FAULT_RESP_RETURN_NEUTRAL,
    .can_timeout     = FAULT_RESP_RETURN_NEUTRAL,
};
```

Passed to FSM via `fsm_step(cfg, inputs, outputs)`. See `fsm.c` for response handling.

## BSPD (Brake System Plausibility Device) Check

FSAE rule EV4.7 requires monitoring for unsafe brake+throttle combination:

**Condition**: Tractive system current sensor detects power draw AND accelerator >25% pedal travel

**Software (VCU) response**: Cut throttle immediately

**Hardware (BSPD circuit) response**: Should never trigger; VCU software check must fire first. If hardware BSPD does trigger → latching fault.

**Recovery**: Motor must stay disabled until accelerator <5% pedal travel

**Software implementation** (`pedal_logic.c`):
- APPS1/APPS2 sensor voltage thresholds calibrated for 25%/5% pedal positions
- Brake pressure sensor monitored for engagement
- If condition detected → `FAULT_PEDAL_PLAUS` set
- FSM response policy determines action (default: RETURN_NEUTRAL)

Thresholds per `VCU_Torque_Safety_Procedures.md`:
- 25% pedal = 1.125V (S1), 2.875V (S2)
- 5% pedal = 0.225V (S1), 3.775V (S2)

**Hardware implementation**: The BSPD circuit monitors current supplied to the tractive system and triggers a `LATCH_FAULT` if load exceeds ~10kW while brakes are engaged. See `DMS-26-VCU-V3.0.pdf` for hardware implementation details.
## CAN Heartbeat Timeouts (FSAE EV8.1.6)

**Inverter Heartbeat** (200ms timeout)
- Inverter must send M170 status message every ~50ms
- If VCU doesn't receive within 200ms → `FAULT_CAN_TIMEOUT`
- Torque cut, FSM transition to fault state

**BMS Heartbeat** (can be configured)
- Similar monitoring for battery management system
- Timeout triggers appropriate fault response

**Rule EV8.1.6 Compliance**
- Vehicles using CAN as safety element must demonstrate shutdown
- If CAN communication interrupted → SDC must open safely
- Alternative: dedicated hardware watchdog with relay output
- Current implementation: software watchdog in VCU with GPIO relay control

See `motor_controller.c`, `can_task.c` for heartbeat tracking.

## Fault Logging

Every fault triggers logging event with context:
- Module: LOG_SRC_FSM or LOG_SRC_SENSOR
- Event: EVT_FAULT_SET
- Payload: fault flags, response policy

Example: `"APPS mismatch fault latched"`

Logs transmitted via USB CDC for debugging. See `vcu-logging.md`.

## Safe Defaults

On boot or fault entry state:
- Throttle disabled (torque command = 0)
- Inverter disabled (no motor power)
- All relays de-energized (safe power state)
- CAN watchdog active (monitor bus health)

FSM enforces these defaults in entry/standby states even if no fault active.

## References

- **Code**: `Core/User/fsm.c`, `Core/User/input_control.c`, `Core/User/pedal_logic.c`, `Core/User/motor_controller.c`
- **FSAE Rules**:
  - EV4.7 - Brake System Plausibility Device (BSPD) requirements
  - EV8.1.6 - CAN bus safety system shutdown demonstration
- **Related Docs**: `vcu-finite-state-machine.md`, `vcu-motor-control.md`, `vcu-sensors.md`, `vcu-watchdog.md`, `VCU_Torque_Safety_Procedures.md`
