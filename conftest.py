"""Pytest configuration for HIL tests."""

import os
import sys
import time

import pytest

from vcu_hil import VcuHil

_DEFAULT_PORT = "COM5" if sys.platform == "win32" else "/dev/vcu"


def pytest_addoption(parser):
    parser.addoption(
        "--port",
        default=os.environ.get("VCU_PORT", _DEFAULT_PORT),
        help="Serial port of VCU device (default: $VCU_PORT or COM5/dev/vcu)",
    )


@pytest.fixture
def vcu(request):
    """VcuHil instance connected to the board, reset and ready."""
    port = request.config.getoption("--port")
    with VcuHil(port) as h:
        time.sleep(0.5)
        h.reset()
        time.sleep(0.1)
        yield h
