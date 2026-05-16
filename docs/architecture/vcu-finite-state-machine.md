# VCU FSM 

## System Overview
The FSM handles the main control logic for the VCU. Vehicle states are indicative of both external and internal factors which impact the vehicle's behaviour. An external factor could be the driver's actions, while internal factors consist typically of faults and signals from other internal systems. 

## System Requirements

1. Control logic must be hardware agnostic
    * Unit/post testing must be performed via injected virtual states
    * The FSM must be tested in the bootup process
    * Input signals must be normalized 

2. Must house all critical vehicle states
    * Must include the entry, standby, neutral, forward, and reverse* states respectively
    * Must clearly label each state using enums

3. Must include telemetry and logging for traceability (see VCU-Logging)
    * State changes must be logged 
    * Must include a serial debug interface 
    * FSM inputs and outputs must be traceable in the event of a fault 

4. Must incorporate redundancy and error handling to avoid catastrophic failure (see VCU-Fault-Handling)
    * Must interface with fault handling system and execute corresponding handling sequence in the event of a fault
    * Must not throw a latching shutdown circuit fault

## Revisions

- 2026-05-16: First draft 
