# VCU Power Distribution

## Overview
Power distribution system manages low-voltage supply to VCU electronics, relay coils, and sensors. Designed for automotive use with redundant fusing and safe power sequencing.

## Requirements

1. **Wide input voltage range support**
   - Nominal: +12V automotive battery
   - Minimum: 8V (discharged battery + voltage drop)
   - Maximum: 16V
   - Must operate reliably across entire range

2. **Regulated supply rails**
   - 3.3V logic supply (STM32, sensors, CAN transceivers)
   - 5V rail (CAN termination, some sensors and auxiliary circuits)
   - Must maintain voltage under full load with worst-case input

3. **Fusing for protection and safety**
   - Each load protected with individual fuse
   - Short circuit on one output cannot disable others
   - Fuse ratings matched to load specifications
   - Automotive blade fuses required (standard, replaceable)

4. **Grounded Low-Voltage System (GLV)**
   - 12V supply acts as "ground" reference for safety circuits
   - Powers shutdown circuit and safety monitoring
   - Must be independent of high-voltage pack
   - Shall be grounded to vehicle chassis

## Input Specifications

**Nominal Supply**: +12V (automotive battery)

**Operating Range**: 8V to 16V
- 8V minimum: cold-crank conditions (discharged battery + voltage drop)
- 16V maximum: alternator output under load transient

**Fusing**: Primary fuse upstream of VCU (vehicle-level protection)

See schematic `DMS-26-VCU-V3.0.pdf` for detailed power tree.

## Power Rails

| Rail          | Voltage | Current Budget | Consumers                          | Fuse |
| ------------- | ------- | -------------- | ---------------------------------- | ---- |
| Logic Supply  | 3.3V    | ~500mA         | STM32, peripherals, sensors        | 1A   |
| Sensor Supply | 5V      | ~200mA         | ADC, accelerator, pressure sensors | 0.5A |


3.3V and 5V rails generated from 12V input via regulators (see `DMS-26-VCU-V3.0.pdf` for power supply details)

## Fused Outputs (Automotive Blade Fuses)

See `DMS-26-VCU-V3.0.pdf` for full specifications. Fuse ratings are subject to change.

**Switched Outputs** (4x relays):
- PWR_INV (inverter power)
- PWR_FANS (cooling fans)
- PWR_BL (brake light)
- PWR_AUX (auxiliary)
 
**Always-On Outputs** (6x relays):
- PWR_ALWAYSON (system power)
- Low-voltage logic rails - 2A Polyfuse

Short circuit on any output trips its fuse, de-energizing that rail while others remain operational.

## Safe Power Sequencing

Boot sequence applies power to subsystems in safe order:

1. **Always-on relay enabled** → Low-voltage logic powered
2. **Inverter relay disabled** → Motor cannot run until FSM ready
3. **Auxiliary outputs disabled** → No unnecessary loads during startup
4. **Watchdog active** → Monitor system health

FSM manages relay timing during state transitions to prevent:
- Voltage spikes from sudden load changes
- Inrush currents from multiple relays energizing simultaneously
- Uncontrolled acceleration from relay chatter

A current shunt on the VCU +VBAT input may be used to monitor system current draw
## DCDC Converter (Optional)

Some vehicle variants include isolated DCDC for high-voltage interface:
- Generates 12V low-voltage rail from 400V tractive system battery pack
- Provides galvanic isolation for safety-critical signals

A DCDC is not currently implemented for DMS-27, however it may be included in future revisions

## Brownout Detection

Note: VCU hw-V3.0 does *not* implement a dedicated reset/brownout for the microcontroller. This section is reserved for future hardware revisions

## Current Monitoring

Shunt resistor on main +VBAT input rail enables current monitoring:
- ADC measures voltage across shunt
- Enables real-time power consumption telemetry
- Useful for battery management system coordination

## References

- **Code**: `Core/User/` (power initialization in startup code and `main.c`)
- **Hardware Schematic**: `DMS-26-VCU-V3.0.pdf`
- **FSAE Rules**: General power supply safety principles (no specific rule; automotive standard practice)
- **Related Docs**: `vcu-gpio.md`, `vcu-boot.md`, `vcu-watchdog.md`
