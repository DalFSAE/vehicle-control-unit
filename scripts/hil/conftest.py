"""Pytest configuration for HIL tests."""

import os

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
        yield h
