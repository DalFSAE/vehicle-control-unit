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

Frame: `[CMD (1 byte)][LEN (1 byte)[payload (LEN bytes)]`


| CMD  | Name      | Payload | Note |
| 0x01 | SPOOF_SET | VcuInputs (packed) | Enable spoofing, and parse payload |
| 0x02 | SPOOF_CLR | (none) | Disable spoofing and return inputs to hardware |

## Python

Python will be used to provide inputs from the PC.

- CLI to allow integration with GH Actions
- CSV input, to allow replay of telemetry or mock sequences
- Logging of VCU outputs overtime
