# VCU Fault Handling

## Overview
Fault handling system detects sensor/subsystem failures and executes safety responses. FSM implements a configurable fault policy that determines whether to cut throttle, return to neutral, trigger shutdown circuit, or latch the fault.

## Requirements

1. **BSPD - Hardware circuit (EV.7.7)**
   - Independent hardware circuit monitors tractive system current and brake engagement
   - If both exceed thresholds simultaneously → latching fault, SDC opens
   - VCU has no role in the BSPD latch/reset sequence
   - VCU software PPC check (req 2) must preempt the hardware BSPD under normal operation
   - See [vcu-shutdown-circuit.md](vcu-shutdown-circuit.md)

2. **Pedal Plausibility Checks (PPC) - Software, run every sensor cycle (~10ms)**

   **a. APPS Agreement** - `FAULT_APPS_DISAGREE`
   - APPS1 and APPS2 are inverse channels: normalize each using their respective voltage range, confirm they agree within 10%
   - Disagreement for >100ms triggers fault

   **b. Brake+Throttle Plausibility** - `FAULT_PEDAL_PLAUS`
   - Throttle >25% AND (FBPS OR RBPS above brake threshold) simultaneously → motor cut
   - Motor remains cut until throttle returns below 5%

   **c. Sensor Range** - `FAULT_SENSOR_RANGE`
   - Each sensor must read within its valid operating range after normalization
   - Out-of-range indicates disconnected or shorted sensor

3. **CAN heartbeat monitoring (FSAE EV8.1.6)**
   - Critical vehicle systems must maintain CAN heartbeats
   - Loss of inverter heartbeat (200ms) → return to neutral
   - Loss of HVC heartbeat → return to neutral or shutdown
   - Required for safety shutdown system compliance
   - See [vcu-watchdog.md](vcu-watchdog.md) for timing details

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
| CAN_TIMEOUT | Communication | Inverter or HVC heartbeat missing | RETURN_NEUTRAL |

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

## BSPD (Brake System Plausibility Device)

The BSPD is a hardware circuit (EV.7.7), independent of the VCU. It monitors tractive system current and brake engagement directly. If both exceed their thresholds simultaneously, the BSPD latches and opens the SDC. This requires a manual reset via the latch button.

The VCU software PPC brake+throttle check (`FAULT_PEDAL_PLAUS`) is intended to detect and cut throttle before the hardware BSPD ever triggers. See [vcu-shutdown-circuit.md](vcu-shutdown-circuit.md) for the hardware circuit details and `DMS-26-VCU-V3.0.pdf` for the hardware implementation.

## CAN Heartbeat Timeouts (FSAE EV8.1.6)

See [vcu-watchdog.md](vcu-watchdog.md) for timeout values and monitoring details.

**Inverter Heartbeat**
- Loss of heartbeat: `FAULT_CAN_TIMEOUT`, torque cut, FSM fault state
- See [vcu-motor-control.md](vcu-motor-control.md) for message details and timing

**HVC Heartbeat** (includes BMS status)
- Timeout triggers appropriate fault response

**Dash Fault Indicators (HVC-controlled)**

The HVC is responsible for driving two dashboard LEDs (BMS Fault, IMD Fault):

| State | Color | Condition |
|-------|-------|-----------|
| OK | Green | No fault, SDC latched |
| Unlatched | Yellow | SDC unlatched at vehicle startup |
| Fault | Flashing Red | Fault thrown by BMS or IMD |

- At startup the SDC is unlatched; LEDs may show yellow until the SDC latches
- LEDs only transition to flashing red if a fault is actively thrown

See [vcu-shutdown-circuit.md](vcu-shutdown-circuit.md) for SDC latch behaviour.

See [vcu-shutdown-circuit.md](vcu-shutdown-circuit.md) for EV8.1.6 compliance details and [vcu-watchdog.md](vcu-watchdog.md) for heartbeat monitoring.

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

- **Code**: `Core/User/fsm.c`, `Core/User/sensor_control.c`, `Core/User/motor_controller.c`
- **FSAE Rules**:
  - EV.7.7 - BSPD hardware circuit
  - EV4.7 - APPS redundancy and pedal plausibility checks
  - EV8.1.6 - CAN bus safety system shutdown demonstration
- **Related Docs**: `vcu-finite-state-machine.md`, `vcu-motor-control.md`, `vcu-sensors.md`, `vcu-watchdog.md`, `VCU_Torque_Safety_Procedures.md`
