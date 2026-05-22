# VCU Motor Control

## Overview
Motor control subsystem manages inverter communication, torque commands, and motor safety. It bridges FSM state decisions with low-voltage PM100DX inverter commands over CAN.

## Requirements

1. **Safe torque command delivery**
   - All torque commands must follow the command structure outlined `motor-controller-fw/docs/CAN Protocol (V6_3).pdf`
   - Cannot enable torque without prior disable command (inverter lockout feature)
   - Torque commands must respect FSAE 80 kW limit
   - Commands must stop if heartbeat lost (200ms timeout)

2. **Inverter heartbeat monitoring**
   - Inverter must transmit status message every ~50ms (see [vcu-fault-handling.md](vcu-fault-handling.md))
   - VCU must monitor receipt and timeout within 200ms
   - Loss of heartbeat triggers immediate torque cut
   - Must integrate with fault handling system

3. **PM100DX compliance (datasheet)**
   - Implement CAN message M192 (command) and M170 (status) per datasheet
   - Support inverter state machine (VSM) monitoring
   - Handle inverter faults (overspeed, overtemp, DC overvoltage)
   - Provide feedback to the driver in case of a fault → display message on dash
   - Enforce inverter enable lockout requirement

4. **Direction control safety**
   - Cannot change motor direction while inverter enabled
   - Sudden reversal disables inverter + re-locks enable
   - Prevents unsafe direction changes during operation

5. **Thread-safe command cache**
   - FSM writes commands to protected mutex cache
   - CAN task reads from cache for transmission
   - Prevents race conditions between control and communication

## Torque Command Flow

FSM → Sensor values → Torque calculation → CAN transmit → Inverter

1. **FSM determines torque intent** based on vehicle state and pedal input
2. **Sensor faults may cut torque** (APPS disagree, implausibility, range errors)
3. **Torque command encoded** as CAN message (M192 Command Message)
4. **CAN transmit** at periodic rate (synchronized with CAN task)
5. **Inverter receives** command and energizes motor if safe

## CAN Command Message (M192)

Frame ID: `CAN0_POWERTRAIN_M192_COMMAND_MESSAGE_FRAME_ID`

Fields:
- `vcu_inv_torque_command` - desired motor torque (Nm)
- `vcu_inv_torque_limit_command` - torque limit for inverter protection
- `vcu_inv_speed_command` - motor speed reference (RPM, optional)
- `vcu_inv_inverter_enable` - enable motor drive (0 = disabled, 1 = enabled)
- `vcu_inv_inverter_discharge` - discharge high voltage (contactor control)
- `vcu_inv_speed_mode_enable` - speed control mode (vs. torque mode)
- `vcu_inv_direction_command` - motor direction (0 = reverse, 1 = forward)
- `vcu_inv_rolling_counter` - incrementing counter for CAN frame validation

See `can0_powertrain.c` for encoding/decoding logic.

## Inverter Heartbeat Monitoring

Inverter must transmit status on CAN at regular interval (~50ms). VCU monitors receipt:
- If heartbeat not received within **200ms**, assume inverter fault
- Torque immediately cut (no new commands sent)
- FSM transitions to fault state
- Vehicle shall return to neutral

Heartbeat tracking in `motor_controller.c`: `s_inv.last_rx_tick_ms`

## Inverter Feedback

Inverter transmits M170 Internal States message with status:
- `inv_vsm_state` - inverter state machine state
- `inv_torque_feedback` - actual motor torque being produced
- Fault flags (post-fault, run-fault)

Processed in `inverter_rx()` callback (ISR context).

## Safety Features

**Inverter Enable Lockout** (PM100DX feature)
- Before sending Enable command, must first send Disable
- Prevents accidental re-enable after fault
- Set via `vcu_inv_inverter_enable` bit transitions

**Sudden Reversal Protection**
- Cannot change motor direction while inverter is enabled
- Attempting reversal disables inverter and re-locks enable

**Torque Limits**
- FSAE rules: max 150 Nm motor output
- `vcu_inv_torque_limit_command` enforced by inverter firmware
- VCU also enforces in FSM logic

**Overspeed, Overtemperature**
- Inverter monitors motor speed and winding temperature
- Auto-shutdown if thresholds exceeded
- Faults reported back on M170 message

See `VCU_Torque_Safety_Procedures.md` for detailed safety specifications.

## Command Cache & Mutex

FSM writes commands to protected cache: `motor_controller_set_cmd()`
CAN task reads from cache: `motor_controller_get_cmd()`

Mutex serializes access (prevents FSM/CAN race conditions).

## References

- **Code**: `Core/User/motor_controller.c`, `Core/User/can0_powertrain.c`, `Core/User/can_task.c`
- **Hardware Datasheet**: PM100DX datasheet `motor-controller-fw/docs/PM and RM Hardware User's Manual (V3_8).pdf`
- **FSAE Rules**: EV4.5 (electrical power steering disabled when not in drive), EV4.7 (torque limits)
- **Related Docs**: `vcu-finite-state-machine.md`, `vcu-fault-handling.md`, `can.md`, `VCU_Torque_Safety_Procedures.md`
