# VCU Sensor Processing

## Overview
Sensor subsystem acquires analog inputs via ADC with DMA, processes them for faults, and publishes normalized values to FSM and CAN bus.

## Requirements

1. **Redundant sensor validation (FSAE EV4.7)**
   - Two independent APPS sensors (S1, S2) must agree within tolerance
   - Both must sum to ~5V (0.5V offset), scale: S1 (0-4.5V), S2 (4.5V-0V)
   - If disagreement detected → trigger APPS_DISAGREE fault
   - Must validate during each sensor read cycle (~10ms)

2. **Brake + Throttle Plausibility Check (FSAE EV4.7)**
   - Cannot have mechanical brakes AND accelerator >25% simultaneously
   - If condition detected → trigger PEDAL_PLAUS fault
   - Motor must stay disabled until throttle <5%

3. **Sensor range monitoring**
   - Each sensor has min/max valid range
   - Out-of-range values indicate disconnected/shorted sensors
   - Range faults trigger SENSOR_RANGE fault
   - Each sensor has a struct which includes conversion factors, and sensor settings.

4. **DMA-based acquisition for responsiveness**
   - Uses ADC with DMA to avoid polling overhead
   - Continuous sampling with rolling buffer
   - Timer-triggered conversion for consistent timing

5. **Support for additional sensors**
   - Wheel speed (Wheel Turtle Pro), suspension potentiometers, steering angle sensor, thermocouple (see [vcu-thermocouple.md](vcu-thermocouple.md))


## Sensor Types

This list should include all sensors that the VCU supports:

| Sensor      | Channel | Range    | Purpose                      | Fault Check         | Datasheet |
| ----------- | ------- | -------- | ---------------------------- | ------------------- | --------- |
| APPS1       | ADC1    | 0.5-4.5V | Accelerator pedal position   | APPS_DISAGREE       |           |
| APPS2       | ADC1    | 4.5-0.5V | Redundant accelerator sensor | APPS_DISAGREE       |           |
| FBPS        | ADC1    | 0.5-4.5V | Front brake pressure         | BRAKE_LIGHT_THRES   |           |
| RBPS        | ADC1    | 0.5-4.5V | Rear brake pressure          | Pressure monitoring |           |
| TSCUR       | ADC     | 0.5-4.5V | Tractive System DC current   | BSPD                |           |
| Wheel Speed | TBD     | TBD      | Wheel speed (Wheel Turtle Pro) | TBD               |           |
| Suspension  | TBD     | TBD      | Suspension potentiometers    | TBD                 |           |
| Steering    | TBD     | TBD      | Steering angle               | TBD                 |           |
| Thermocouple | TBD    | TBD      | Temperature monitoring       | TBD                 | [vcu-thermocouple.md](vcu-thermocouple.md) |

Note: All ADC inputs include a 20k/10k resistor divider, scales 5V signal to 1.67V (Vout = Vin × 10k/30k)

## ADC Configuration

- **Peripheral**: ADC1 with DMA (Direct Memory Access)
- **Resolution**: 12-bit (0-4095 counts)
- **Buffer**: 8 samples for rolling average
- **Sampling**: Continuous, DMA fills buffer without CPU polling
- **Timer**: TIM2 triggers conversions at regular interval

ADC raw counts converted to voltage: `voltage = (counts / 4096.0) * 3.3V`

See `sensor_control.c` for acquisition logic.

## Fault Detection

**APPS Disagreement** (`FAULT_APPS_DISAGREE`)
- Redundant APPS1/APPS2 sensors must agree within tolerance
- If discrepancy detected: log fault, request fault response
- Response policy: `cfg->apps_disagree` (default: CUT_THROTTLE)

**Pedal Implausibility** (`FAULT_PEDAL_PLAUS`)
- APPS and brake sensor check: cannot be both high simultaneously
- FSAE rule EV4.7: if brake engaged AND throttle >25%, power must cut
- Motor must stay cut until throttle <5%
- Response policy: `cfg->pedal_plaus` (default: RETURN_NEUTRAL)

**Sensor Range** (`FAULT_SENSOR_RANGE`)
- Each sensor has min/max valid range
- Out-of-range values indicate disconnected/shorted sensors
- Response policy: `cfg->sensor_range` (default: RETURN_NEUTRAL)

See `input_control.c` for plausibility logic, `pedal_logic.c` for APPS/brake rules. See also [vcu-fault-handling.md](vcu-fault-handling.md).

## Data Publishing

Normalized sensor values published after processing:
- `sensor_get_throttle()` → FSM normalized pedal position (0-1)
- `sensor_get_brake()` → boolean brake engaged signal
- `sensor_get_fault_flags()` → bit field of active faults

All ADC telemetry transmitted on CAN0 (see `can0_powertrain.c`).

## Sensor Task

Runs at priority AboveNormal to ensure responsive fault detection:
- Acquires ADC data
- Performs plausibility checks
- Updates fault flags
- Publishes normalized outputs
- Period: ~10ms

## References

- **Code**: `Core/User/sensor_control.c`, `Core/User/input_control.c`, `Core/User/pedal_logic.c`
- **FSAE Rules**: Rule EV4.7 (Brake System Plausibility Device) - both APPS sensor redundancy and brake/throttle checking
- **Related Docs**: `vcu-motor-control.md`, `vcu-fault-handling.md`, `VCU_Torque_Safety_Procedures.md`
