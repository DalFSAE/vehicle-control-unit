# VCU Runtime Environment 


## App Layer

Highest firmware level


## Subsystems

Subsystems for the VCU firmware.

### Vehicle FSM

Main authority for the overall vehicle state, including: 

- Includes driver inputs,
- relay control.
### Torque Control

Real time algorithm for controlling the tractive system motor controller.

Publishes to CAN

See also `DalFSAE/motor-controller-fw`


### ADC

Primary interface for VCU analog inputs.

- Uses direct memory access (DMA)
- Publishes data, to be used by other subsystems
- All ADC data shall be published to CAN0 

## Interfaces

How subsystems can interact with each other

### CAN

The VCU acts as the primary controller for the vehicles main CAN network. Two CAN buses are available:

- CAN0
- CAN1

See also `DalFSAE/motor-controller-fw` and `DalFSAE/can-db`.

### GPIO


