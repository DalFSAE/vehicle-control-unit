#!/usr/bin/env python3
"""
vcu_hil.py - Serial transport layer for VCU USB CDC binary protocol

Provides VcuHil class for sending/receiving binary command frames and helpers.
"""

import logging
import re
import struct
import time
from dataclasses import dataclass, field
from typing import List, Optional

import serial

_STM32_LOG_RE = re.compile(rb'\[\s*\d+\][^\n]*\n')
_vcu_log = logging.getLogger("vcu")


# Command IDs (must match UsbCmd_t in usb_cmd.h)
CMD_SPOOF_SET       = 0x01
CMD_SPOOF_CLEAR     = 0x02
CMD_REQUEST_OUTPUTS = 0x03
CMD_REQUEST_STATE   = 0x04
CMD_STEP            = 0x05
CMD_FAULT_INJECT    = 0x06
CMD_RESET           = 0x07
CMD_ECHO            = 0x45

# FSM states (must match FsmState_t in fsm.h)
ST_ENTRY   = 0
ST_STANDBY = 1
ST_NEUTRAL = 2
ST_FORWARD = 3
ST_REVERSE = 4

# Fault flags (must match FaultFlags_t in vcu_io.h)
FAULT_NONE          = 0
FAULT_APPS_DISAGREE = (1 << 0)
FAULT_PEDAL_PLAUS   = (1 << 1)
FAULT_SENSOR_RANGE  = (1 << 2)
FAULT_CAN_TIMEOUT   = (1 << 3)

# debug_cmd bitfield (must match vcu_io.h comment)
DBG_LED1 = (1 << 0)
DBG_LED2 = (1 << 1)
DBG_LED3 = (1 << 2)


_VCUOUTPUTS_FMT = '<??????B?fIB'  # 17 bytes, must match packed VcuOutputs in vcu_io.h


@dataclass
class VcuOutputs:
    """VCU output state (17 bytes packed, must match C struct in vcu_io.h)"""
    relay_always_on: bool = False
    relay_inverter: bool = False
    brake_light: bool = False
    mc_brake_sw: bool = False
    can_watchdog: bool = False
    tssi_en: bool = False
    motor_direction: int = 0
    throttle_enabled: bool = False
    throttle_request: float = 0.0
    buzzer_beep_ms: int = 0
    debug_leds: int = 0           # bitfield: DBG_LED1 | DBG_LED2 | DBG_LED3

    @classmethod
    def unpack(cls, data: bytes) -> 'VcuOutputs':
        fields = struct.unpack_from(_VCUOUTPUTS_FMT, data)
        return cls(*fields)


@dataclass
class VcuInputs:
    """VCU input state (14 bytes packed, must match C struct in vcu_io.h)"""
    fault_flags: int = 0
    throttle_request: float = 0.0
    brake_pressed: bool = False
    rtd_button: bool = False
    fwrd_switch: bool = False
    rvrs_switch: bool = False
    ts_active: bool = False
    debug_cmd: int = 0          # bitfield: DBG_LED1 | DBG_LED2 | DBG_LED3

    def pack(self) -> bytes:
        """Pack into 14-byte binary format"""
        return struct.pack(
            '<If?????B',
            self.fault_flags,
            self.throttle_request,
            self.brake_pressed,
            self.rtd_button,
            self.fwrd_switch,
            self.rvrs_switch,
            self.ts_active,
            self.debug_cmd,
        )


class VcuHil:
    """Hardware-in-the-loop interface to VCU via USB CDC serial"""

    def __init__(self, port: str, baud: int = 115200, timeout: float = 1.0):
        """
        Connect to VCU on specified serial port.

        Args:
            port: Serial port (e.g., '/dev/ttyACM0', 'COM3')
            baud: Baud rate (default 115200)
            timeout: Read timeout in seconds
        """
        self.ser = serial.Serial(port, baud, timeout=timeout, write_timeout=None)
        self.captured_logs: List[str] = []
        time.sleep(1.0)

    def send_cmd(self, cmd: int, payload: bytes = b'') -> None:
        """
        Send a binary command frame.

        Frame: [CMD (1 byte)][LEN (1 byte)][payload (LEN bytes)]

        Args:
            cmd: Command ID
            payload: Command payload (max 255 bytes)
        """
        if len(payload) > 255:
            raise ValueError(f"Payload too large: {len(payload)} > 255")
        pending = self.ser.read_all()
        if pending:
            self._drain_logs(pending)
        self.ser.reset_input_buffer()
        frame = bytes([cmd, len(payload)]) + payload
        self.ser.write(frame)

    def _drain_logs(self, buf: bytes) -> bytes:
        """
        Strip STM32 log lines from buf (lines matching [timestamp] ...).
        Stripped lines are appended to self.captured_logs.
        Returns the remaining binary bytes.
        """
        result = b''
        pos = 0
        while pos < len(buf):
            m = _STM32_LOG_RE.search(buf, pos)
            if m is None:
                result += buf[pos:]
                break
            result += buf[pos : m.start()]
            line = m.group().decode('ascii', errors='replace').rstrip()
            self.captured_logs.append(line)
            _vcu_log.debug(line)
            pos = m.end()
        return result

    def recv(self, timeout: Optional[float] = None) -> bytes:
        """
        Receive bytes from VCU, stripping any interleaved STM32 log lines.

        Args:
            timeout: Override read timeout for this call

        Returns:
            Received bytes with log lines removed (logs stored in captured_logs)
        """
        old_timeout = self.ser.timeout
        if timeout is not None:
            self.ser.timeout = timeout
        try:
            data = self.ser.read(1024)
            return self._drain_logs(data)
        finally:
            if timeout is not None:
                self.ser.timeout = old_timeout

    def echo(self, data: bytes) -> bytes:
        """
        Send echo command and receive response.

        Args:
            data: Data to echo

        Returns:
            Echoed data
        """
        self.send_cmd(CMD_ECHO, data)
        time.sleep(0.01)
        return self.recv(timeout=0.5)

    def spoof_inputs(self, inputs: VcuInputs) -> None:
        """Inject spoof inputs into the VCU."""
        self.send_cmd(CMD_SPOOF_SET, inputs.pack())
        time.sleep(0.05)

    def clear_spoof(self) -> None:
        """Clear input spoofing (resume reading hardware)."""
        self.send_cmd(CMD_SPOOF_CLEAR, b'')

    def request_state(self) -> int:
        """
        Poll current FSM state.

        Returns:
            FsmState_t value (ST_ENTRY, ST_STANDBY, etc.)
        """
        self.send_cmd(CMD_REQUEST_STATE, b'')
        time.sleep(0.05)
        resp = self.recv(timeout=0.5)
        # Response format: [0x84, 0x01, state_value]
        if len(resp) >= 3 and resp[0] == 0x84 and resp[1] == 0x01:
            return resp[2]
        raise ValueError(f"Invalid REQUEST_STATE response: {resp.hex()}")

    def request_outputs(self) -> VcuOutputs:
        """Poll current VcuOutputs from the device."""
        self.send_cmd(CMD_REQUEST_OUTPUTS, b'')
        time.sleep(0.01)
        resp = self.recv(timeout=0.5)
        if len(resp) >= 2 and resp[0] == 0x83:
            payload_len = resp[1]
            payload = resp[2 : 2 + payload_len]
            return VcuOutputs.unpack(payload)
        raise ValueError(f"Invalid REQUEST_OUTPUTS response: {resp.hex()}")

    def set_debug_cmd(self, cmd: int) -> None:
        """Spoof debug command bits (e.g. DBG_LED1 | DBG_LED3) into the next FSM step."""
        self.spoof_inputs(VcuInputs(debug_cmd=cmd))

    def step(self) -> None:
        """Manually advance FSM one cycle (step mode)."""
        self.send_cmd(CMD_STEP, b'')

    def fault_inject(self, flags: int) -> None:
        """
        Inject fault flags for testing.

        Args:
            flags: Bitmask of fault flags (FAULT_CAN_TIMEOUT, etc.)
        """
        payload = struct.pack('<I', flags)
        self.send_cmd(CMD_FAULT_INJECT, payload)

    def reset(self) -> None:
        """Reset FSM to ST_ENTRY."""
        self.send_cmd(CMD_RESET, b'')
        time.sleep(0.05)

    def close(self) -> None:
        """Close serial connection."""
        if self.ser.is_open:
            self.ser.close()
        time.sleep(0.2)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
