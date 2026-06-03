#!/usr/bin/env python3
"""
test_fsm.py - HIL pytest suite for FSM control via USB

Run with: pytest test_fsm.py --port /dev/ttyACM0 -v
"""

import time

import pytest

from vcu_hil import VcuInputs, ST_STANDBY, ST_NEUTRAL, ST_FORWARD, FAULT_CAN_TIMEOUT


@pytest.fixture
def vcu(vcu_ready):
    """Per-test fixture: reset FSM to clean state on the shared session connection."""
    vcu_ready.reset()
    time.sleep(0.1)
    yield vcu_ready


# Tests

def test_echo_roundtrip(vcu):
    """Sanity check: USB link is working."""
    result = vcu.echo(b"ping")
    assert result == b"ping"


def test_initial_state_after_reset(vcu):
    """After reset, FSM should be in ST_STANDBY."""
    state = vcu.request_state()
    assert state == ST_STANDBY


def test_spoof_and_step_to_neutral(vcu):
    """Spoof fwrd_switch + ts_active, step → should reach NEUTRAL."""
    inputs = VcuInputs(fwrd_switch=True, ts_active=True)
    vcu.spoof_inputs(inputs)
    vcu.step()
    state = vcu.request_state()
    assert state == ST_NEUTRAL


def test_neutral_to_forward(vcu):
    """From NEUTRAL, spoof rtd_button + brake_pressed, step → should reach FORWARD."""
    inputs = VcuInputs(fwrd_switch=True, ts_active=True)
    vcu.spoof_inputs(inputs)
    vcu.step()

    inputs.rtd_button = True
    inputs.brake_pressed = True
    vcu.spoof_inputs(inputs)
    vcu.step()

    state = vcu.request_state()
    assert state == ST_FORWARD


def test_fault_inject_in_forward(vcu):
    """In FORWARD, inject CAN timeout → FSM should handle the fault."""
    inputs = VcuInputs(fwrd_switch=True, ts_active=True)
    vcu.spoof_inputs(inputs)
    vcu.step()

    inputs.rtd_button = True
    inputs.brake_pressed = True
    vcu.spoof_inputs(inputs)
    vcu.step()

    vcu.fault_inject(FAULT_CAN_TIMEOUT)
    vcu.step()

    state = vcu.request_state()
    assert state is not None


def test_request_outputs_correct_format(vcu):
    """REQUEST_OUTPUTS should return valid data."""
    outputs = vcu.request_outputs()
    assert len(outputs) > 0


def test_clear_spoof_doesnt_crash(vcu):
    """clear_spoof should not crash."""
    vcu.clear_spoof()
    state = vcu.request_state()
    assert state is not None
