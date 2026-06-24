# README

Documenation for the vehicle-control-unit, and its firmware.

## Getting Started

The DalFSAE Vehicle Control Unit (VCU) is an STM32F407 based embedded system, which controls the DMS electrical system.

## Architecture

For hardware schematics, see [DMS-26-VCU-V3.0.pdf](../DMS-26-VCU-V3.0.pdf)


## Communication

- CAN0: primary CAN bus
- CAN1: reserved for future use
- Serial: for debug infomation
- USB: st-link - connected to main microcontroller

see also:


## Torque Control

see also:


## Power

- 8V to 16V input (+VBAT)
- All outputs fused (automotive blade style)
    - 4x switched
    - 6x always-on
