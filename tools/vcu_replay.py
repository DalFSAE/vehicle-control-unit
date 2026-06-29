#!/usr/bin/env python3
"""
vcu_replay.py - Replay a pre-normalized telemetry CSV against real VCU firmware.

Accepts a CSV with columns matching VcuInputs fields (see --help for schema).
Drives the firmware over USB CDC via VcuHil, capturing the full VcuOutputs frame
each tick. Writes an output CSV and optionally shows plots.

Usage:
    python tools/vcu_replay.py --csv run.csv --port COM5 --output results/run.csv
"""

import argparse
import dataclasses
import logging
import os
import sys
import time
import warnings

import pandas as pd

from vcu_hil import VcuHil, VcuInputs, VcuOutputs, ST_ENTRY, ST_STANDBY, ST_NEUTRAL, ST_FORWARD, ST_REVERSE

_STATE_NAMES = {
    ST_ENTRY:   "ENTRY",
    ST_STANDBY: "STANDBY",
    ST_NEUTRAL: "NEUTRAL",
    ST_FORWARD: "FORWARD",
    ST_REVERSE: "REVERSE",
}

log = logging.getLogger(__name__)

_BOOL_FIELDS  = {"brake_pressed", "rtd_button", "fwrd_switch", "rvrs_switch", "ts_active"}
_INT_FIELDS   = {"fault_flags", "debug_cmd"}
_FLOAT_FIELDS = {"throttle_request"}
_ALL_INPUT_FIELDS = _BOOL_FIELDS | _INT_FIELDS | _FLOAT_FIELDS


def _parse_bool(val) -> bool:
    if isinstance(val, bool):
        return val
    if isinstance(val, (int, float)):
        return bool(val)
    s = str(val).strip().lower()
    if s in ("1", "true", "yes"):
        return True
    if s in ("0", "false", "no"):
        return False
    raise ValueError(f"Cannot parse bool from: {val!r}")


def load_csv(path: str) -> pd.DataFrame:
    """Load and lightly validate a replay CSV. Returns the DataFrame."""
    df = pd.read_csv(path)
    if "time_s" not in df.columns:
        raise ValueError(f"CSV must have a 'time_s' column. Found: {list(df.columns)}")
    unknown = set(df.columns) - {"time_s"} - _ALL_INPUT_FIELDS
    if unknown:
        warnings.warn(f"Ignoring unknown CSV columns: {sorted(unknown)}", stacklevel=2)
    return df


def row_to_inputs(row: pd.Series) -> VcuInputs:
    """Map a DataFrame row to a VcuInputs, using safe defaults for missing columns."""
    defaults = dataclasses.asdict(VcuInputs())
    kwargs = {}
    for field in _BOOL_FIELDS:
        if field in row.index:
            kwargs[field] = _parse_bool(row[field])
    for field in _INT_FIELDS:
        if field in row.index:
            kwargs[field] = int(row[field])
    for field in _FLOAT_FIELDS:
        if field in row.index:
            kwargs[field] = float(row[field])
    return VcuInputs(**{**defaults, **kwargs})


def _record(time_s: float, inputs: VcuInputs, outputs: VcuOutputs, state: int) -> dict:
    """Flatten a tick's inputs, outputs, and FSM state into a single dict for CSV output."""
    d = {"time_s": time_s, "fsm_state": state, "fsm_state_name": _STATE_NAMES.get(state, str(state))}
    for f in dataclasses.fields(inputs):
        d[f"in_{f.name}"] = getattr(inputs, f.name)
    for f in dataclasses.fields(outputs):
        d[f"out_{f.name}"] = getattr(outputs, f.name)
    return d


def run_replay(
    vcu: VcuHil,
    df: pd.DataFrame,
    *,
    mode: str = "step",
) -> list[dict]:
    """
    Replay all rows against the VCU. Returns a list of per-tick records.

    Args:
        vcu:  Connected VcuHil instance. Caller owns reset/close.
        df:   DataFrame from load_csv().
        mode: "step" - one STEP per row, no wall-clock delay (default).
              "timed" - sleep the inter-row time_s delta before each row (best-effort).
    """
    records = []
    prev_time: float | None = None

    for _, row in df.iterrows():
        t = float(row["time_s"])

        if mode == "timed" and prev_time is not None:
            dt = t - prev_time
            if dt > 0:
                time.sleep(dt)

        inputs = row_to_inputs(row)
        vcu.spoof_inputs(inputs)
        vcu.step()
        outputs = vcu.request_outputs()
        state   = vcu.request_state()
        records.append(_record(t, inputs, outputs, state))

        log.debug("t=%.3f  state=%-7s  throttle=%.3f  throttle_enabled=%s",
                  t, _STATE_NAMES.get(state, state), inputs.throttle_request, outputs.throttle_enabled)
        prev_time = t

    vcu.clear_spoof()
    return records


def save_results(records: list[dict], path: str) -> None:
    """Write captured records to a CSV file."""
    pd.DataFrame(records).to_csv(path, index=False)
    log.info("Saved %d rows to %s", len(records), path)


def plot_results(records: list[dict]) -> None:
    """Plot the full system state over time, includng inputs, relays, motor, faults."""
    import matplotlib.pyplot as plt

    df = pd.DataFrame(records)
    fig, axes = plt.subplots(6, 1, figsize=(14, 16), sharex=True)
    fig.suptitle("VCU Replay - System State", fontsize=13)

    # --- FSM State ---
    ax = axes[0]
    ax.step(df["time_s"], df["fsm_state"], where="post", color="black", linewidth=1.5)
    ax.set_yticks(list(_STATE_NAMES.keys()))
    ax.set_yticklabels(list(_STATE_NAMES.values()))
    ax.set_ylabel("FSM State")
    ax.grid(True, axis="x")

    # --- Throttle ---
    ax = axes[1]
    ax.plot(df["time_s"], df["in_throttle_request"],  label="in",  color="steelblue")
    ax.plot(df["time_s"], df["out_throttle_request"], label="out", color="orange", linestyle="--")
    ax.plot(df["time_s"], df["out_throttle_enabled"].astype(int), label="enabled", color="green", linestyle=":")
    ax.set_ylabel("Throttle")
    ax.set_ylim(-0.05, 1.1)
    ax.legend(loc="upper right")
    ax.grid(True)

    # --- Driver inputs ---
    ax = axes[2]
    ax.plot(df["time_s"], df["in_brake_pressed"].astype(int), label="brake_pressed", color="crimson")
    ax.plot(df["time_s"], df["in_fwrd_switch"].astype(int),   label="fwrd_switch",   color="royalblue",  linestyle="--")
    ax.plot(df["time_s"], df["in_ts_active"].astype(int),     label="ts_active",     color="teal",       linestyle="-.")
    ax.plot(df["time_s"], df["in_rtd_button"].astype(int),    label="rtd_button",    color="darkorange",  linestyle=":")
    ax.set_ylabel("Driver Inputs")
    ax.set_ylim(-0.1, 1.4)
    ax.legend(loc="upper right")
    ax.grid(True)

    # --- Power / Relays ---
    ax = axes[3]
    ax.plot(df["time_s"], df["out_relay_always_on"].astype(int), label="relay_always_on", color="navy")
    ax.plot(df["time_s"], df["out_relay_inverter"].astype(int),  label="relay_inverter",  color="purple",    linestyle="--")
    ax.plot(df["time_s"], df["out_tssi_en"].astype(int),         label="tssi_en",         color="teal",      linestyle="-.")
    ax.set_ylabel("Relays")
    ax.set_ylim(-0.1, 1.4)
    ax.legend(loc="upper right")
    ax.grid(True)

    # --- Motor ---
    ax = axes[4]
    ax.plot(df["time_s"], df["out_motor_direction"],           label="motor_direction (0=fwd)",  color="seagreen")
    ax.plot(df["time_s"], df["out_brake_light"].astype(int),   label="brake_light",              color="crimson",  linestyle="--")
    ax.plot(df["time_s"], df["out_mc_brake_sw"].astype(int),   label="mc_brake_sw",              color="salmon",   linestyle=":")
    ax.plot(df["time_s"], df["out_buzzer_beep_ms"] / max(df["out_buzzer_beep_ms"].max(), 1),
            label="buzzer (norm)", color="goldenrod", linestyle="-.")
    ax.set_ylabel("Motor / Brakes")
    ax.set_ylim(-0.1, 1.4)
    ax.legend(loc="upper right")
    ax.grid(True)

    # --- Faults ---
    ax = axes[5]
    ax.plot(df["time_s"], df["in_fault_flags"],               label="fault_flags (in)",  color="red")
    ax.plot(df["time_s"], df["out_can_watchdog"].astype(int),  label="can_watchdog",      color="gray",   linestyle="--")
    ax.plot(df["time_s"], df["out_debug_leds"],                label="debug_leds",        color="olive",  linestyle=":")
    ax.set_ylabel("Faults")
    ax.set_xlabel("Time (s)")
    ax.legend(loc="upper right")
    ax.grid(True)

    plt.tight_layout()
    plt.show()


def main(argv=None) -> int:
    _default_port = os.environ.get("VCU_PORT", "COM5" if sys.platform == "win32" else "/dev/vcu")

    parser = argparse.ArgumentParser(
        description="Replay a pre-normalized telemetry CSV against VCU firmware over USB CDC.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
CSV schema (columns map 1:1 to VcuInputs; all optional except time_s):
  time_s           float  - row timestamp in seconds
  throttle_request float  - [0..1]
  brake_pressed    bool   - 0/1 or true/false
  rtd_button       bool
  fwrd_switch      bool
  rvrs_switch      bool
  ts_active        bool
  fault_flags      int    - bitmask
  debug_cmd        int    - bitmask

Unknown columns are ignored with a warning.
FSM bring-up is the caller's responsibility - include fwrd_switch/ts_active/rtd_button
rows before sending throttle if ST_FORWARD is required.
        """,
    )

    parser.add_argument("--csv",    required=True,   help="Input replay CSV")
    parser.add_argument("--port",   default=_default_port, help=f"Serial port (default: $VCU_PORT or {_default_port})")
    parser.add_argument("--output", default=None,    help="Output CSV path for captured results")
    parser.add_argument("--mode",   choices=["step", "timed"], default="step",
                        help="step: one STEP per row, no delay (default); timed: honour time_s deltas")
    parser.add_argument("--plot",   action="store_true", help="Show output plots after run")
    parser.add_argument("--verbose", "-v", action="store_true")

    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(name)-12s %(message)s",
    )

    df = load_csv(args.csv)
    log.info("Loaded %d rows from %s", len(df), args.csv)

    with VcuHil(args.port) as vcu:
        vcu.reset()
        records = run_replay(vcu, df, mode=args.mode)

    log.info("Replay complete: %d ticks captured", len(records))

    if args.output:
        save_results(records, args.output)

    if args.plot:
        plot_results(records)

    return 0


if __name__ == "__main__":
    sys.exit(main())
