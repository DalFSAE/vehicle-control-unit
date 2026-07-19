#!/usr/bin/env python3
"""Unit tests for vcu_replay.py - no hardware required."""

import io
import textwrap
import warnings
from unittest.mock import MagicMock, call, patch

import pandas as pd
import pytest

from vcu_hil import VcuInputs, VcuOutputs
from vcu_replay import _parse_bool, _record, load_csv, row_to_inputs, run_replay


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_df(*rows: dict) -> pd.DataFrame:
    """Build a DataFrame from dicts; always includes time_s."""
    return pd.DataFrame(rows)


def _fake_outputs(**kwargs) -> VcuOutputs:
    return VcuOutputs(**kwargs)


def _fake_vcu(outputs: VcuOutputs | None = None, state: int = 1) -> MagicMock:
    """Return a mock VcuHil whose request_outputs/request_state return fixed values."""
    vcu = MagicMock()
    vcu.request_outputs.return_value = outputs or VcuOutputs()
    vcu.request_state.return_value = state
    return vcu


# ---------------------------------------------------------------------------
# _parse_bool
# ---------------------------------------------------------------------------

class TestParseBool:
    def test_int_1_is_true(self):       assert _parse_bool(1) is True
    def test_int_0_is_false(self):      assert _parse_bool(0) is False
    def test_float_1_is_true(self):     assert _parse_bool(1.0) is True
    def test_float_0_is_false(self):    assert _parse_bool(0.0) is False
    def test_bool_true(self):           assert _parse_bool(True) is True
    def test_bool_false(self):          assert _parse_bool(False) is False
    def test_string_true(self):         assert _parse_bool("true") is True
    def test_string_false(self):        assert _parse_bool("false") is False
    def test_string_1(self):            assert _parse_bool("1") is True
    def test_string_0(self):            assert _parse_bool("0") is False
    def test_string_yes(self):          assert _parse_bool("yes") is True
    def test_string_no(self):           assert _parse_bool("no") is False
    def test_string_case_insensitive(self): assert _parse_bool("TRUE") is True

    def test_invalid_raises(self):
        with pytest.raises(ValueError):
            _parse_bool("maybe")


# ---------------------------------------------------------------------------
# load_csv
# ---------------------------------------------------------------------------

class TestLoadCsv:
    def _write_tmp(self, tmp_path, content: str) -> str:
        p = tmp_path / "test.csv"
        p.write_text(textwrap.dedent(content))
        return str(p)

    def test_valid_csv_returns_dataframe(self, tmp_path):
        path = self._write_tmp(tmp_path, """\
            time_s,throttle_request
            0.0,0.0
            0.1,0.5
        """)
        df = load_csv(path)
        assert isinstance(df, pd.DataFrame)
        assert len(df) == 2

    def test_missing_time_s_raises(self, tmp_path):
        path = self._write_tmp(tmp_path, """\
            throttle_request
            0.5
        """)
        with pytest.raises(ValueError, match="time_s"):
            load_csv(path)

    def test_unknown_columns_warn(self, tmp_path):
        path = self._write_tmp(tmp_path, """\
            time_s,unknown_col
            0.0,123
        """)
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            load_csv(path)
        assert any("unknown_col" in str(w.message) for w in caught)

    def test_known_columns_no_warning(self, tmp_path):
        path = self._write_tmp(tmp_path, """\
            time_s,throttle_request,brake_pressed
            0.0,0.5,0
        """)
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            load_csv(path)
        assert not caught


# ---------------------------------------------------------------------------
# row_to_inputs
# ---------------------------------------------------------------------------

class TestRowToInputs:
    def test_missing_columns_use_defaults(self):
        row = pd.Series({"time_s": 0.0})
        inputs = row_to_inputs(row)
        defaults = VcuInputs()
        assert inputs == defaults

    def test_throttle_mapped(self):
        row = pd.Series({"time_s": 0.0, "throttle_request": 0.75})
        assert abs(row_to_inputs(row).throttle_request - 0.75) < 1e-6

    def test_brake_int_1(self):
        row = pd.Series({"time_s": 0.0, "brake_pressed": 1})
        assert row_to_inputs(row).brake_pressed is True

    def test_brake_int_0(self):
        row = pd.Series({"time_s": 0.0, "brake_pressed": 0})
        assert row_to_inputs(row).brake_pressed is False

    def test_brake_string_true(self):
        row = pd.Series({"time_s": 0.0, "brake_pressed": "true"})
        assert row_to_inputs(row).brake_pressed is True

    def test_fwrd_switch_mapped(self):
        row = pd.Series({"time_s": 0.0, "fwrd_switch": 1})
        assert row_to_inputs(row).fwrd_switch is True

    def test_fault_flags_int(self):
        row = pd.Series({"time_s": 0.0, "fault_flags": 5})
        assert row_to_inputs(row).fault_flags == 5

    def test_debug_cmd_int(self):
        row = pd.Series({"time_s": 0.0, "debug_cmd": 0x03})
        assert row_to_inputs(row).debug_cmd == 0x03

    def test_all_fields_together(self):
        row = pd.Series({
            "time_s": 1.0,
            "throttle_request": 0.5,
            "brake_pressed": 1,
            "rtd_button": 0,
            "fwrd_switch": 1,
            "rvrs_switch": 0,
            "ts_active": 1,
            "fault_flags": 2,
            "debug_cmd": 1,
        })
        inp = row_to_inputs(row)
        assert abs(inp.throttle_request - 0.5) < 1e-6
        assert inp.brake_pressed is True
        assert inp.fwrd_switch is True
        assert inp.ts_active is True
        assert inp.fault_flags == 2
        assert inp.debug_cmd == 1


# ---------------------------------------------------------------------------
# _record
# ---------------------------------------------------------------------------

class TestRecord:
    def test_contains_time_s(self):
        r = _record(1.23, VcuInputs(), VcuOutputs(), 1)
        assert r["time_s"] == 1.23

    def test_contains_fsm_state(self):
        from vcu_hil import ST_FORWARD
        r = _record(0.0, VcuInputs(), VcuOutputs(), ST_FORWARD)
        assert r["fsm_state"] == ST_FORWARD
        assert r["fsm_state_name"] == "FORWARD"

    def test_inputs_prefixed_with_in(self):
        r = _record(0.0, VcuInputs(throttle_request=0.4), VcuOutputs(), 1)
        assert abs(r["in_throttle_request"] - 0.4) < 1e-6

    def test_outputs_prefixed_with_out(self):
        r = _record(0.0, VcuInputs(), VcuOutputs(throttle_enabled=True), 1)
        assert r["out_throttle_enabled"] is True

    def test_no_field_overlap(self):
        r = _record(0.0, VcuInputs(), VcuOutputs(), 1)
        # throttle_request appears in both structs; prefixes must distinguish them
        assert "in_throttle_request" in r
        assert "out_throttle_request" in r

    def test_all_input_fields_present(self):
        r = _record(0.0, VcuInputs(), VcuOutputs(), 1)
        import dataclasses
        for f in dataclasses.fields(VcuInputs()):
            assert f"in_{f.name}" in r

    def test_all_output_fields_present(self):
        r = _record(0.0, VcuInputs(), VcuOutputs(), 1)
        import dataclasses
        for f in dataclasses.fields(VcuOutputs()):
            assert f"out_{f.name}" in r


# ---------------------------------------------------------------------------
# run_replay
# ---------------------------------------------------------------------------

class TestRunReplay:
    @pytest.fixture(autouse=True)
    def no_sleep(self, monkeypatch):
        monkeypatch.setattr("vcu_replay.time.sleep", lambda *a: None)

    def test_step_called_once_per_row(self):
        vcu = _fake_vcu()
        df = _make_df(
            {"time_s": 0.0, "throttle_request": 0.0},
            {"time_s": 0.1, "throttle_request": 0.5},
        )
        run_replay(vcu, df)
        assert vcu.step.call_count == 2

    def test_spoof_called_with_correct_inputs(self):
        vcu = _fake_vcu()
        df = _make_df({"time_s": 0.0, "throttle_request": 0.8})
        run_replay(vcu, df)
        sent: VcuInputs = vcu.spoof_inputs.call_args[0][0]
        assert abs(sent.throttle_request - 0.8) < 1e-6

    def test_clear_spoof_called_after_loop(self):
        vcu = _fake_vcu()
        df = _make_df({"time_s": 0.0})
        run_replay(vcu, df)
        vcu.clear_spoof.assert_called_once()

    def test_returns_one_record_per_row(self):
        vcu = _fake_vcu()
        df = _make_df(
            {"time_s": 0.0},
            {"time_s": 0.1},
            {"time_s": 0.2},
        )
        records = run_replay(vcu, df)
        assert len(records) == 3

    def test_record_contains_time_s(self):
        vcu = _fake_vcu()
        df = _make_df({"time_s": 1.5})
        records = run_replay(vcu, df)
        assert records[0]["time_s"] == 1.5

    def test_record_contains_output_fields(self):
        vcu = _fake_vcu(_fake_outputs(throttle_enabled=True))
        df = _make_df({"time_s": 0.0})
        records = run_replay(vcu, df)
        assert records[0]["out_throttle_enabled"] is True

    def test_step_mode_no_sleep(self, monkeypatch):
        sleep_calls = []
        monkeypatch.setattr("vcu_replay.time.sleep", lambda dt: sleep_calls.append(dt))
        vcu = _fake_vcu()
        df = _make_df({"time_s": 0.0}, {"time_s": 1.0})
        run_replay(vcu, df, mode="step")
        assert sleep_calls == []

    def test_timed_mode_sleeps_delta(self, monkeypatch):
        sleep_calls = []
        monkeypatch.setattr("vcu_replay.time.sleep", lambda dt: sleep_calls.append(dt))
        vcu = _fake_vcu()
        df = _make_df({"time_s": 0.0}, {"time_s": 0.5}, {"time_s": 1.0})
        run_replay(vcu, df, mode="timed")
        assert len(sleep_calls) == 2
        assert abs(sleep_calls[0] - 0.5) < 1e-9
        assert abs(sleep_calls[1] - 0.5) < 1e-9

    def test_clear_spoof_called_even_on_empty_df(self):
        vcu = _fake_vcu()
        df = pd.DataFrame(columns=["time_s"])
        run_replay(vcu, df)
        vcu.clear_spoof.assert_called_once()
