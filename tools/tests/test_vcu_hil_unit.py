#!/usr/bin/env python3
"""Unit tests for vcu_hil.py — no hardware required, serial port is mocked."""

import struct
from unittest.mock import MagicMock, patch

import pytest

from vcu_hil import (
    VcuHil,
    VcuInputs,
    CMD_SPOOF_SET,
    CMD_SPOOF_CLEAR,
    CMD_REQUEST_OUTPUTS,
    CMD_REQUEST_STATE,
    CMD_STEP,
    CMD_FAULT_INJECT,
    CMD_RESET,
    CMD_ECHO,
    ST_ENTRY,
    ST_STANDBY,
    ST_NEUTRAL,
    ST_FORWARD,
    ST_REVERSE,
    FAULT_NONE,
    FAULT_APPS_DISAGREE,
    FAULT_PEDAL_PLAUS,
    FAULT_SENSOR_RANGE,
    FAULT_CAN_TIMEOUT,
)


@pytest.fixture(autouse=True)
def no_sleep(monkeypatch):
    monkeypatch.setattr("vcu_hil.time.sleep", lambda *a, **kw: None)


@pytest.fixture
def mock_serial():
    ser = MagicMock()
    ser.is_open = True
    ser.timeout = 1.0
    return ser


@pytest.fixture
def hil(mock_serial):
    with patch("vcu_hil.serial.Serial", return_value=mock_serial):
        yield VcuHil("COM_FAKE")


class TestVcuInputsPack:
    def test_default_pack_is_14_bytes(self):
        assert len(VcuInputs().pack()) == 14

    def test_debug_cmd_encoded(self):
        data = VcuInputs(debug_cmd=0x07).pack()
        assert data[13] == 0x07

    def test_fault_flags_encoded(self):
        data = VcuInputs(fault_flags=0xDEAD).pack()
        assert struct.unpack_from("<I", data, 0)[0] == 0xDEAD

    def test_throttle_encoded(self):
        data = VcuInputs(throttle_request=0.5).pack()
        assert abs(struct.unpack_from("<f", data, 4)[0] - 0.5) < 1e-6

    def test_zero_throttle_encoded(self):
        data = VcuInputs(throttle_request=0.0).pack()
        assert struct.unpack_from("<f", data, 4)[0] == 0.0

    def test_booleans_encoded_at_correct_offsets(self):
        data = VcuInputs(
            brake_pressed=True,
            rtd_button=False,
            fwrd_switch=True,
            rvrs_switch=False,
            ts_active=True,
        ).pack()
        assert data[8] == 1   # brake_pressed
        assert data[9] == 0   # rtd_button
        assert data[10] == 1  # fwrd_switch
        assert data[11] == 0  # rvrs_switch
        assert data[12] == 1  # ts_active

    def test_all_booleans_false_by_default(self):
        data = VcuInputs().pack()
        assert data[8:13] == b'\x00\x00\x00\x00\x00'


class TestSendCmd:
    def test_frame_has_cmd_len_payload(self, hil, mock_serial):
        hil.send_cmd(0x01, b'\xAA\xBB')
        mock_serial.write.assert_called_once_with(bytes([0x01, 0x02, 0xAA, 0xBB]))

    def test_empty_payload_sends_two_bytes(self, hil, mock_serial):
        hil.send_cmd(CMD_RESET)
        mock_serial.write.assert_called_once_with(bytes([CMD_RESET, 0x00]))

    def test_payload_too_large_raises(self, hil):
        with pytest.raises(ValueError):
            hil.send_cmd(0x01, b'\x00' * 256)

    def test_exactly_255_bytes_is_ok(self, hil, mock_serial):
        hil.send_cmd(0x01, b'\x00' * 255)
        mock_serial.write.assert_called_once()

    def test_drains_pending_bytes_before_write(self, hil, mock_serial):
        mock_serial.read_all.return_value = b'[1] INFO stale\n'
        hil.send_cmd(0x01, b'')
        mock_serial.read_all.assert_called_once()
        mock_serial.write.assert_called_once()

    def test_echo_command_id(self, hil, mock_serial):
        mock_serial.read.return_value = b"hello"
        hil.send_cmd(CMD_ECHO, b"hello")
        frame = mock_serial.write.call_args[0][0]
        assert frame[0] == CMD_ECHO


class TestRequestState:
    def test_parses_standby(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0x84, 0x01, ST_STANDBY])
        assert hil.request_state() == ST_STANDBY

    def test_parses_neutral(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0x84, 0x01, ST_NEUTRAL])
        assert hil.request_state() == ST_NEUTRAL

    def test_parses_forward(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0x84, 0x01, ST_FORWARD])
        assert hil.request_state() == ST_FORWARD

    def test_parses_entry(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0x84, 0x01, ST_ENTRY])
        assert hil.request_state() == ST_ENTRY

    def test_raises_on_wrong_response_byte(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0xFF, 0x01, ST_STANDBY])
        with pytest.raises(ValueError):
            hil.request_state()

    def test_raises_when_second_byte_wrong(self, hil, mock_serial):
        # Tightened check: resp[1] must be 0x01
        mock_serial.read.return_value = bytes([0x84, 0x02, ST_STANDBY])
        with pytest.raises(ValueError):
            hil.request_state()

    def test_raises_on_empty_response(self, hil, mock_serial):
        mock_serial.read.return_value = b''
        with pytest.raises(ValueError):
            hil.request_state()

    def test_raises_on_too_short_response(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0x84, 0x01])
        with pytest.raises(ValueError):
            hil.request_state()

    def test_sends_request_state_command(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0x84, 0x01, ST_STANDBY])
        hil.request_state()
        frame = mock_serial.write.call_args[0][0]
        assert frame[0] == CMD_REQUEST_STATE
        assert frame[1] == 0x00  # no payload


def _make_outputs_response(**kwargs):
    """Build a mock REQUEST_OUTPUTS response with a valid VcuOutputs payload."""
    from vcu_hil import VcuOutputs, _VCUOUTPUTS_FMT
    out = VcuOutputs(**kwargs)
    payload = struct.pack(
        _VCUOUTPUTS_FMT,
        out.relay_always_on, out.relay_inverter, out.brake_light,
        out.mc_brake_sw, out.can_watchdog, out.tssi_en,
        out.motor_direction, out.throttle_enabled,
        out.throttle_request, out.buzzer_beep_ms, out.debug_leds,
    )
    return bytes([0x83, len(payload)]) + payload


class TestRequestOutputs:
    def test_returns_vcuoutputs(self, hil, mock_serial):
        from vcu_hil import VcuOutputs
        mock_serial.read.return_value = _make_outputs_response()
        assert isinstance(hil.request_outputs(), VcuOutputs)

    def test_parses_fields(self, hil, mock_serial):
        mock_serial.read.return_value = _make_outputs_response(
            throttle_enabled=True, throttle_request=0.5, debug_leds=0x03
        )
        out = hil.request_outputs()
        assert out.throttle_enabled is True
        assert abs(out.throttle_request - 0.5) < 1e-6
        assert out.debug_leds == 0x03

    def test_raises_on_wrong_response_byte(self, hil, mock_serial):
        mock_serial.read.return_value = bytes([0xFF, 0x04]) + b'\x00' * 4
        with pytest.raises(ValueError):
            hil.request_outputs()

    def test_raises_on_empty_response(self, hil, mock_serial):
        mock_serial.read.return_value = b''
        with pytest.raises(ValueError):
            hil.request_outputs()

    def test_sends_request_outputs_command(self, hil, mock_serial):
        mock_serial.read.return_value = _make_outputs_response()
        hil.request_outputs()
        frame = mock_serial.write.call_args[0][0]
        assert frame[0] == CMD_REQUEST_OUTPUTS


class TestFaultInject:
    def test_sends_fault_flags_as_uint32(self, hil, mock_serial):
        hil.fault_inject(FAULT_CAN_TIMEOUT)
        frame = mock_serial.write.call_args[0][0]
        assert frame[0] == CMD_FAULT_INJECT
        payload = frame[2:]
        assert struct.unpack("<I", payload)[0] == FAULT_CAN_TIMEOUT

    def test_combined_flags(self, hil, mock_serial):
        flags = FAULT_APPS_DISAGREE | FAULT_SENSOR_RANGE
        hil.fault_inject(flags)
        frame = mock_serial.write.call_args[0][0]
        assert struct.unpack("<I", frame[2:])[0] == flags


class TestClose:
    def test_closes_open_port(self, hil, mock_serial):
        mock_serial.is_open = True
        hil.close()
        mock_serial.close.assert_called_once()

    def test_skips_close_when_already_closed(self, hil, mock_serial):
        mock_serial.is_open = False
        hil.close()
        mock_serial.close.assert_not_called()

    def test_context_manager_closes_on_exit(self, mock_serial):
        with patch("vcu_hil.serial.Serial", return_value=mock_serial):
            with VcuHil("COM_FAKE"):
                pass
        mock_serial.close.assert_called_once()


class TestCommandConstants:
    def test_all_command_ids_are_unique(self):
        ids = [
            CMD_SPOOF_SET, CMD_SPOOF_CLEAR, CMD_REQUEST_OUTPUTS,
            CMD_REQUEST_STATE, CMD_STEP, CMD_FAULT_INJECT, CMD_RESET, CMD_ECHO,
        ]
        assert len(ids) == len(set(ids))

    def test_fault_flags_are_distinct_bits(self):
        flags = [FAULT_APPS_DISAGREE, FAULT_PEDAL_PLAUS, FAULT_SENSOR_RANGE, FAULT_CAN_TIMEOUT]
        combined = 0
        for f in flags:
            assert f != 0
            assert combined & f == 0, f"Flag {f:#x} overlaps with already-set bits"
            combined |= f

    def test_fault_none_is_zero(self):
        assert FAULT_NONE == 0

    def test_state_constants_are_unique(self):
        states = [ST_ENTRY, ST_STANDBY, ST_NEUTRAL, ST_FORWARD, ST_REVERSE]
        assert len(states) == len(set(states))
