"""Pytest configuration for HIL tests."""

import os

import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--port",
        default=os.environ.get("VCU_PORT", "/dev/vcu"),
        help="Serial port of VCU device (default: $VCU_PORT or /dev/vcu)",
    )
