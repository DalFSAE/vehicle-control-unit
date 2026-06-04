# VCU Telemetry Replay

## Summary

Replay recorded AiM telemetry laps against real VCU firmware over USB CDC. Feeds `VcuInputs` from real race data into the spoof path and captures the full output frame each tick. Enables algorithm validation and fault scenario testing without the full vehicle.

This repo owns the replay tool and USB protocol library (`tools/replay/`). `DalFSAE/data-and-telemetry` is a consumer.

## Architecture

AiM CSV is loaded via `TelemetryReplayScenario` (in `data-and-telemetry`), which normalizes sensor channels into `VcuInputs`. The replay tool drives the firmware over USB CDC via `protocol.py`, capturing the full `VcuOutputs` frame each tick and writing results to CSV and plots.

## `protocol.py`

Standalone importable library. Owns USB CDC framing and command encoding.

```python
conn = VcuConnection(port="COM3")
conn.spoof_set(inputs: VcuInputs)      # inject packed inputs
conn.step()                             # advance FSM one cycle
conn.request_outputs() -> VcuOutputs   # full output frame
conn.spoof_clear()                      # restore hardware reads on exit
```

`VcuInputs` packing must match the firmware `__attribute__((packed))` byte struct (`vcu_io.h`). Unset fields use safe defaults. Frame format and command IDs are in `vcu-input-spoof.md`.

Output capture records the full `VcuOutputs` frame, allowing for other hardware captures in the future.

## Replay Modes

| Mode | Use case | Timing |
|------|----------|--------|
| **Step-locked** | FSM-only tests, verify state machine logic per ticket | One `STEP` per input row, no clock timing |
| **Timed** | Hardware-in-the-loop, e.g. 1s throttle step, observe motor response | Inputs sent at original sample rate, real elapsed time |

## References

- **Code**: `tools/replay/` (this repo), `DalFSAE/data-and-telemetry/src/vehicle/scenarios/telemetry_replay.py`
- **Related Docs**: `vcu-input-spoof.md`, `vcu-finite-state-machine.md`, `vcu-sensors.md`
