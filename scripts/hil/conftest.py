"""Pytest configuration for HIL tests."""

import pytest


def pytest_addoption(parser):
    parser.addoption("--port", default=None, help="Serial port of VCU device")
