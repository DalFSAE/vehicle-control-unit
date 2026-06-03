"""Pytest configuration for HIL tests."""

import pytest


def pytest_addoption(parser):
    parser.addoption("--port", default=None, help="Serial port of VCU device")


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    rep = outcome.get_result()
    setattr(item, f"rep_{rep.when}", rep)
