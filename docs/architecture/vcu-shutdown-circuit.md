# VCU Shutdown Circuit

## Overview

The Shutdown Circuit (SDC) is a hardware series loop that must be closed for the Tractive System to operate. When any element opens, the AIRs (Accumulator Isolation Relays) de-energize and HV power is cut. The VCU participates via a single GPIO relay (`SDC_RELAY`) and monitors SDC state via CAN from the HVC.

## SDC Series Loop

Per FSAE EV.7.1.1, the following are connected in series:

1. HVC eFuse (SDC source, controlled by HVC)
2. BMS (latching relay, EV.7.3)
3. IMD (latching relay, EV.7.6)
4. BSPD (latching relay, EV.7.7)
5. Interlocks, physical connectors, MSD, BOTS (EV.7.8)
6. Master Switches, GLVMS and TSMS (EV.7.9)
7. Shutdown Buttons, three BRBs cockpit and both sides (EV.7.10)
8. VCU SDC_RELAY (software-controlled, non-latching)

The loop returns to the HVC, which uses SDC voltage to drive the AIRs.

## VCU Role

The VCU contributes one non-latching element to the SDC: `SDC_RELAY` (GPIO output, active-close).

| Condition | SDC_RELAY State |
|-----------|----------------|
| Boot / default | De-energized (SDC open) |
| Normal operation | Energized (SDC closed) |
| CAN heartbeat loss (EV.8.1.6) | De-energized (SDC opens) |
| Fault cleared | Re-energized automatically |

The VCU does not open the SDC for software BSPD/PPC faults; those use `RETURN_NEUTRAL` or `CUT_THROTTLE` responses. Hardware BSPD, BMS, and IMD each have independent latching circuits.

## Latching Behaviour

Hardware latching faults (BMS, IMD, BSPD):
- Require manual reset via the LATCH button on the side of the car
- Per EV.7.2.3, shutdown buttons or TSMS operation must not re-close these faults
- The HVC monitors latch state; the VCU has no role in the latch/reset sequence

VCU fault (CAN heartbeat loss):
- Non-latching; VCU automatically re-energizes `SDC_RELAY` once CAN heartbeat resumes
- No manual intervention required

## SDC State Feedback

The HVC broadcasts SDC state on CAN. The VCU reads this to confirm the SDC is closed before permitting Ready-To-Drive and to log SDC state changes for diagnostics.

## FSAE EV.8.1.6 Compliance

EV.8.1.6 requires that vehicles using CAN as a safety element demonstrate shutdown equivalent to BRB operation if CAN is interrupted.

The VCU software watchdog monitors CAN heartbeats (inverter, BMS). On heartbeat timeout, the VCU de-energizes `SDC_RELAY`, opening its section of the SDC loop and cutting tractive power. There is no dedicated hardware CAN watchdog circuit. This must be demonstrable at Electrical Technical Inspection.

## Requirements

1. `SDC_RELAY` must default to de-energized; SDC is open on boot until the VCU is ready
2. VCU must confirm SDC is closed via HVC CAN before permitting RTD
3. CAN heartbeat loss must open `SDC_RELAY` within the timeout defined in `vcu-watchdog.md`
4. VCU must not interfere with the hardware latch/reset sequence
5. `SDC_RELAY` must re-close automatically once the fault condition clears

## References

- **Code**: `Core/User/board_outputs.c`, `Core/User/motor_controller.c`, `Core/User/can_task.c`
- **Hardware**: `DMS-26-VCU-V3.0.pdf`
- **FSAE Rules**: EV.7.1, EV.7.2, EV.7.7, EV.8.1.6
- **Related Docs**: `vcu-fault-handling.md`, `vcu-watchdog.md`, `vcu-gpio.md`
