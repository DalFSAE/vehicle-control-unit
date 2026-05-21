# VCU GPIO Outputs

## Overview
GPIO subsystem controls low-voltage relays, indicator lights, and buzzer. All outputs are safety-critical and default to de-energized (safe state) on boot.

## Requirements

1. **Safe power defaults**
   - All outputs de-energized on power-up and reset
   - No uncontrolled relay activation possible
   - Fail-safe: relay coils are normally open

2. **Relay control via FSM**
   - FSM maintains output state machine (VcuOutputs struct)
   - Relay energization only in specific FSM states
   - State transitions must be atomic (no partial state)
   - Always-on relay powers low-voltage logic continuously

3. **Fused outputs (automotive blade fuses)**
   - Each output has individual fuse for short-circuit protection
   - Fuse trip de-energizes that output while others remain operational
   - Fuse ratings match load specifications (see schematic for details)

4. **Tractive System Status Indicator (TSSI)**
   - Audible/visual indicator when high-voltage system active
   - Required by FSAE rules for safety at technical inspection
   - Buzzer control via FSM timing (1000ms beep during RTD button press)
   - TSSI may be controlled by software
   - Activation of the light is handled by the High Voltage Controller
   - The TSSI (and by extension the vehicle) may be either `GREEN` for `OKAY`, or `RED` for `BMS FAULT` or `IMD FAULT`

5. **GPIO configuration for embedded systems**
   - Push-pull output mode (active drive high/low)
   - No pull-ups (external relay drivers provide pull down resistor)
   - Low frequency (relays not high-speed switching)

## Output Map

| Output       | Pin                 | Purpose                                       | Default State |
| ------------ | ------------------- | --------------------------------------------- | ------------- |
| PWR_ALWAYSON | PWR_ALWAYSON_Pin    | Always-on relay (always supplies low voltage) | ENABLED       |
| PWR_INV      | PWR_INV_Pin         | Inverter power relay (motor driver)           | DISABLED      |
| PWR_BL       | PWR_BL_GPIO_Port    | Brake light relay                             | DISABLED      |
| PWR_FANS     | PWR_FANS_GPIO_Port  | Cooling fan relay                             | DISABLED      |
| PWR_AUX      | PWR_AUX_GPIO_Port   | Auxiliary power relay                         | DISABLED      |
| SDC_RELAY    | SDC_RELAY_GPIO_Port | Shutdown circuit relay (high voltage)         | DISABLED      |
| TSSI         | RESERVED            | Tractive System Status Indicator (buzzer)     | DISABLED      |
| LD3-LD6      | LD*_Pin             | Debug LEDs (development only)                 | OFF           |

See hardware schematic: `DMS-26-VCU-V3.0.pdf`

## Relay Control

All relays controlled via `board_outputs.c`:

```c
void board_output_set(OutputChannel_t ch, bool value);      // Set relay state
void board_output_enable(OutputChannel_t ch);               // Energize relay
void board_output_disable(OutputChannel_t ch);              // De-energize relay
uint32_t board_output_get_state(OutputChannel_t ch);        // Query state
```

GPIO initialization during boot:
- All outputs configured as GPIO_MODE_OUTPUT_PP (push-pull)
- All pins reset to GPIO_PIN_RESET (relay de-energized)
- No pull-up/pull-down (external relay drivers provide pulls)

## FSM Output Control

FSM state machine controls relay outputs via `VcuOutputs` struct:

- `out->relay_always_on` - always-on power relay (enabled in most states)
- `out->relay_inverter` - inverter power (enabled only when motor can run)
- `out->throttle_enabled` - allow torque commands to inverter
- `out->brake_light_enabled` - brake light feedback

FSM guarantees safe defaults:
- ENTRY state: enables always-on and TSSI, disables throttle
- STANDBY: disables inverter, throttle, TSSI
- NEUTRAL/FORWARD/REVERSE: manage based on sensor inputs and faults

## Fusing

All switched outputs fused at VCU (automotive blade fuses):
- 4 switched fuses (FANS, AUX, BL, INV)
- 6 always-on fuses (system power rails)
- Short circuit or overload trips fuse, de-energizes output safely

See schematic for fuse ratings and locations.

## Buzzer/TSSI Control

TSSI (Tractive System Status Indicator) is a buzzer indicating high-voltage system active:

- Driven from FSM's `out->buzzer_beep_ms` field
- During RTD (Ready-To-Drive) button press: 1000ms buzzer beep
- Notifies driver that high voltage is energized
- Implemented in `output_control.c`

## Safe Power Sequencing

Boot sequence powers up subsystems in safe order:
1. Always-on relay enabled (powers low-voltage logic)
2. Inverter relay disabled (no motor power until FSM ready)
3. Brake light, fans, aux disabled (not needed until drive state)
4. SDC relay disabled (HV disconnected until vehicle ready)

FSM transitions manage relay timing to prevent voltage spikes or inrush currents.

## References

- **Code**: `Core/User/board_outputs.c`, `Core/User/output_control.c`, `Core/User/dio.c`
- **Lecture Notes**: See [[202501310158 Vehicle Control Unit]] for general GPIO architecture
- **FSAE Rules**: General output safety principles; TSSI required per competition rules
- **Hardware**: `DMS-26-VCU-V3.0.pdf` (schematic with fuse ratings and pin assignments)
- **Related Docs**: `vcu-boot.md`, `vcu-finite-state-machine.md`, `vcu-power-distribution.md`
