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
    """
    Session-scoped fixture that waits for the firmware's ===HIL_READY===
    sentinel before any test runs.

    This drains all on-boot self-test output (banners, Unity text, etc.) so
    that the USB channel is clean when individual tests open their own
    connections.  The fixture is a no-op if the board already booted (the
    sentinel never re-appears).

    All test fixtures that talk to the VCU should depend on this fixture
    (directly or via the function-scoped ``vcu`` fixture in test_fsm.py).
    """
    port = request.config.getoption("--port")
    with VcuHil(port) as h:
        ready = h.wait_for_ready(timeout=30.0)
        if not ready:
            # Non-fatal: board was already past boot when we connected.
            pass
    # Yield nothing — the value is unused; callers just need the wait done.
    yield
