# VCU Input Spoofing

# Summary

The VCU needs a way to inject `VcuInputs` from a host PC over USB CDC for hardware-in-the-loop (HIL) testing. This lets scripts control the state machine and its inputs through various sequences, allowing us to test various sequences, without needed access to the full vehicle hardware.


# Architecture

- Input: PC (Python script / serial)
  - Optional: pandas/csv dataframe
- Transmit over USB CDC (pyserial) and captured by `CDC_Receive_FS()`
- `usb_cmd_rx(buf, len)` new command buffer, which parses USB commands.

## Data Structure

Input and output of the state machine is controlled by the following structs. The VCU input is packed, which statically asserts the byte structure. 

```c
// vcu_io.h
// Inputs consumed by the FSM each cycle, assembled by vcu_gather_inputs().
// Packed so it can be spoofed wholesale over USB for HIL testing.
typedef struct __attribute__((packed)) {...} VcuInputs;
_Static_assert(sizeof(VcuInputs) == 13, "VcuInputs size mismatch");

// Outputs produced by the FSM each cycle, applied by vcu_apply_outputs().
typedef struct {...} VcuOutputs;
```


## Command Protocol

Frame format: `[CMD (1 byte)][LEN (1 byte)][payload (LEN bytes)]`

Response format (for commands that return data): `[CMD | 0x80 (1 byte)][optional fields]`
The response command ID is the request command ID bitwise OR'd with 0x80, allowing the host to correlate responses with requests.

| CMD  | Name            | Payload          | Response | Note |
|------|-----------------|------------------|----------|------|
| 0x01 | SPOOF_SET       | 13 bytes (VcuInputs) | none | Enable spoofing with injected inputs |
| 0x02 | SPOOF_CLEAR     | none             | none | Disable spoofing; resume hardware reads |
| 0x03 | REQUEST_OUTPUTS | none             | `[0x83][sizeof(VcuOutputs)][VcuOutputs]` | Poll current FSM outputs |
| 0x04 | REQUEST_STATE   | none             | `[0x84][0x01][state_byte]` | Poll current FSM state (ST_ENTRY=0, ST_STANDBY=1, etc.) |
| 0x05 | STEP            | none             | none | Manually advance FSM one cycle (step mode) |
| 0x06 | FAULT_INJECT    | 4 bytes (uint32_t flags) | none | Merge fault flags into spoof inputs |
| 0x07 | RESET           | none             | none | Reset FSM to ST_ENTRY, clear spoof, clear step mode |
| 0x45 | ECHO            | arbitrary bytes  | echoed bytes | Loopback for link testing |

## Python

Python will be used to provide inputs from the PC.

- CLI to allow integration with GH Actions
- CSV input, to allow replay of telemetry or mock sequences
- Logging of VCU outputs overtime
