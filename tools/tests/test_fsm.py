#!/usr/bin/env python3
"""
test_fsm.py - HIL pytest suite for FSM control via USB

Run with: pytest test_fsm.py --port /dev/ttyACM0 -v
"""

import time

from vcu_hil import VcuInputs, ST_STANDBY, ST_NEUTRAL, ST_FORWARD, FAULT_CAN_TIMEOUT, DBG_LED1, DBG_LED2, DBG_LED3


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
    """REQUEST_OUTPUTS should return a parsed VcuOutputs."""
    from vcu_hil import VcuOutputs
    outputs = vcu.request_outputs()
    assert isinstance(outputs, VcuOutputs)


def test_clear_spoof_doesnt_crash(vcu):
    """clear_spoof should not crash."""
    vcu.clear_spoof()
    state = vcu.request_state()
    assert state is not None

def test_led_pattern(vcu):
    """debug_cmd input should be reflected in debug_leds output after a step."""
    debug_cmd = DBG_LED1 | DBG_LED3
    vcu.set_debug_cmd(debug_cmd)
    vcu.step()
    outputs = vcu.request_outputs()
    assert outputs.debug_leds == debug_cmd

def test_printing_outputs(vcu):
    """Test that printing outputs doesn't crash."""
    outputs = vcu.request_outputs()
    print(outputs)


def test_inputs_match_outputs(vcu):
    """Spoofing fwrd_switch+ts_active and stepping should transition to NEUTRAL."""
    inputs = VcuInputs(fwrd_switch=True, ts_active=True)
    vcu.spoof_inputs(inputs)
    vcu.step()
    assert vcu.request_state() == ST_NEUTRAL
    