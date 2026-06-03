"""Pytest configuration for HIL tests."""

import os
import time

import pytest

from vcu_hil import VcuHil


def pytest_addoption(parser):
    parser.addoption(
        "--port",
        default=os.environ.get("VCU_PORT", "/dev/vcu"),
        help="Serial port of VCU device (default: $VCU_PORT or /dev/vcu)",
    )


@pytest.fixture(scope="session")
def vcu_ready(request):
    """Open port once per session, drain boot output, and keep connection alive for all tests."""
    port = request.config.getoption("--port")
    with VcuHil(port) as h:
        h.wait_for_ready(timeout=30.0)
        # Let the VCU settle after boot tests: drain any trailing output then
        # wait for the channel to go quiet before handing control to tests.
        time.sleep(0.5)
        h.drain_input(idle_timeout=0.5)
        yield h
