# CAN Network Design

See also `DalFSAE/can-db` for full documentation.

## Nodes

### can0

- VCU
- INV
- BMS
- HVC
- DASH
- EMETER
- CHRG

### can1

Reserved for future use

# Software Controlled CAN Termination



### Intro



* The CAN schematic can be found on sheet 6 out of 7 in DMS-27/ELECTRICAL/VCU/SCHEMATICS/DMS-26-VCU-V3.0.pdf.
* CAN1 and CAN2 are functionally identical and schematic components will be referred to by the name they have in CAN1.
* Manual hardware CAN termination exists through jumper J2 in series with 120Ω resistor R66, but utilizing this requires physical access to the board. GPIO-controlled termination allows termination state to be changed remotely. This is useful for debugging and testing without needing to open the car.



### Circuit

* CANH (CAN High) and CANL (CAN Low) feed into 62Ω resistors, connected in the middle by a 4.7nF capacitor that passes to ground. The 62Ω resistors in series equal \~120Ω, the industry standard.
* The 62Ω resistors R64 and R65 provide impedance that matches the CAN bus characteristic impedance (62Ωx2 = \~120Ω industry standard), preventing signal reflections at the end of the transmission line.
* The capacitor C10 at the midpoint of R64 and R65 creates a filter for common-mode noise, improving electromagnetic interference rejection.
* Two split resistors joined by a capacitor in the midpoint leading to GND is called a "split termination circuit"
* U7/U8 are Vishay VORA1010M4 optically-isolated solid-state relays. Each is wired as a simple electronic switch: GPIO-driven EN input controls whether IN connects through to OUT. When enabled, this completes the ground-return path for the corresponding 62Ω termination resistor; when disabled, that resistor is left floating and contributes no termination.
* The optical isolation is a property of the part (no direct electrical connection between the control/GPIO side and the switched side) but is incidental here — its functional role in this circuit is purely an electronically-controlled switch for the termination network.

#### Firmware

* it can be seen in vehicle-control-unit\\Core\\Src\\gpio.c that CAN1\_TERMINATION\_Pin and CAN2\_TERMINATION\_Pin are both initialized to 0 on boot, and both pins are on port E
* in vehicle-control-unit\\Core\\Inc\\main.h CAN1\_TERMINATION\_Pin is defined as GPIO\_PIN\_7, and CAN1\_TERMINATION\_GPIO\_PORT is defined as GPIOE
* in vehicle-control-unit\\Core\\Inc\\main.h CAN2\_TERMINATION\_Pin is defined as GPIO\_PIN\_8, and CAN2\_TERMINATION\_GPIO\_Port is defined as GPIOE
* To conclude, CAN1 termination is on PE7 and CAN2 termination is on PE8

#### Connection

* Driving PE7/PE8 high enables U7/U8, connecting the corresponding termination resistor to ground; driving them low disconnects it. Since both pins reset to 0 at boot, termination is disabled by default unless firmware explicitly enables it.



