# Summary

Plan for the VCU logging and telemetry interfaces 

## Logging

Logging is critical for understanding what the vehicle is doing.

Logging is human/event-oriented:

- “Precharge started”
- “APPS mismatch fault latched”
- “Inverter enable denied: IMD fault active”

## Telemetry

Structured periodic data:

- APPS1 = 1.82 V
- Brake pressure = 420 psi
- FSM state = DRIVE_READY
- Relay states bitmap = 0x13

Properties:

- fixed-format
- deterministic
- periodic
- mostly CAN

## Goals

- [ ] Provide rich text logs during firmware development 

## Topic

A `topic` acts as a simple publish/subscribe interface. 

Specs:

- No callbacks available
- 

## Serial (UART)

- Used for verbose logging
- Provides "print" style logging 
- Typically connected to host machine for testing

## CAN0

Primary vehicle CAN bus

## CAN1

Reserved for future use

