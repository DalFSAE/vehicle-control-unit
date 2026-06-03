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
    """Drain boot-test output once per session before any test opens a connection."""
    port = request.config.getoption("--port")
    with VcuHil(port) as h:
        h.wait_for_ready(timeout=30.0)
    yield
