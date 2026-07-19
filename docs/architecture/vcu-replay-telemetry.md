# VCU Telemetry Replay

## Summary

Replay a CSV of vehicle inputs against real VCU firmware over USB, capture outputs each tick, and write results to CSV and plots. Useful for testing algorithms and fault scenarios without the full vehicle.

## Architecture

`tools/vcu_replay.py` reads a CSV, sends each row to the firmware as `VcuInputs` via `tools/vcu_hil.py`, and records the full `VcuOutputs` response. `DalFSAE/data-and-telemetry` can produce input CSVs from real AiM sessions.

## Input CSV Schema

Columns match `VcuInputs` fields. Only `time_s` is required; missing columns use safe defaults. Unknown columns are ignored with a warning.

| Column | Type | Notes |
|--------|------|-------|
| `time_s` | float | Required |
| `throttle_request` | float | [0..1] |
| `brake_pressed` | bool | 0/1 or true/false |
| `rtd_button`, `fwrd_switch`, `rvrs_switch`, `ts_active` | bool | |
| `fault_flags` | int | Bitmask — see `vcu_io.h` |
| `debug_cmd` | int | Bitmask |

The FSM starts in `ST_STANDBY` after reset. Include `fwrd_switch`/`ts_active`/`rtd_button` rows before sending throttle to reach `ST_FORWARD`.

## Replay Modes

| Mode | Use case | Timing |
|------|----------|--------|
| **Step-locked** (default) | FSM logic tests | One `STEP` per row, no delay |
| **Timed** | Hardware response tests, e.g. throttle step | Sleeps the `time_s` delta between rows |

## CLI

```
python tools/vcu_replay.py --csv run.csv --port COM5 --output results/run.csv [--mode step|timed] [--plot]
```

Port defaults to `$VCU_PORT`, then `COM5` (Windows) / `/dev/vcu` (Linux).

## References

- **Code**: `tools/vcu_replay.py`, `tools/vcu_hil.py`, `tools/tests/test_vcu_replay.py`
- **Related Docs**: `vcu-input-spoof.md`, `vcu-finite-state-machine.md`, `vcu-sensors.md`
