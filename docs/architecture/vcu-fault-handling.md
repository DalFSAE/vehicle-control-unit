# VCU Fault Handling

## Overview
Fault handling system detects sensor/subsystem failures and executes safety responses. FSM implements a configurable fault policy that determines whether to cut throttle, return to neutral, trigger shutdown circuit, or latch the fault.

## Requirements

1. **FSAE EV4.7 Brake System Plausibility Device (BSPD)**
   - Must detect brake + tractive system current simultaneous engagement
   - Brake sensor AND accelerator >25% pedal → immediate shutdown
   - Motor shall stay cut until accelerator <5% pedal travel
   - Shall be demonstrable at technical inspection
   - Shall be implemented in hardware

2. **Sensor redundancy validation**
   - Two independent accelerator sensors (APPS1, APPS2) must agree
   - Must sum to ~5V (0.5V offset) or flag as fault
   - Disagreement → APPS_DISAGREE fault with configurable response

3. **CAN heartbeat monitoring (FSAE EV8.1.6)**
   - Critical vehicle systems must maintain CAN heartbeats
   - Loss of inverter heartbeat (200ms) → immediate torque cut
   - Loss of BMS/HVC heartbeat → return to neutral or shutdown
   - Required for safety shutdown system compliance

4. **Configurable fault responses**
   - Not all faults require same action
   - Some faults warrant throttle cut, others require state change
   - Responses must be adjustable without code changes
   - Must support latching faults for driver awareness

5. **Fault logging and traceability**
   - Every fault must be logged with context
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

**Condition**: Mechanical brakes engaged AND accelerator >25% pedal travel

**Action**: Power to motor must immediately and completely shut down

**Recovery**: Motor must stay disabled until accelerator <5% pedal travel

Implemented in `pedal_logic.c`:
- APPS1/APPS2 sensor voltage thresholds calibrated for 25%/5% pedal positions
- Brake pressure sensor monitored for engagement
- If condition detected → `FAULT_PEDAL_PLAUS` set
- FSM response policy determines action (default: RETURN_NEUTRAL)

Thresholds per `VCU_Torque_Safety_Procedures.md`:
- 25% pedal = 1.125V (S1), 2.875V (S2)
- 5% pedal = 0.225V (S1), 3.775V (S2)

**Note:** The core BSPD logic is implemented in hardware, and causes a `LATCH_FAULT`. The BSPD triggers whenever the *current supplied to the tractive system* exceeded ~10kW. The VCU's internal monitoring systems should trigger before the hardware monitor is able to. See also `vehicle-control-unit/DMS-26-VCU-V3.0.pdf` for more info on the BSPD hardware implementation.
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
