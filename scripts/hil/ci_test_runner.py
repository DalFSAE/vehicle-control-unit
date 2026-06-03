#!/usr/bin/env python3
"""
CI-ready test runner for HIL tests on GitHub Actions self-hosted runner.

Detects VCU device, runs pytest suite, handles timeouts and errors gracefully.
"""

import sys
import time
import subprocess
from pathlib import Path
from typing import Optional
import serial.tools.list_ports


def find_vcu_port(timeout_s: int = 10, default_port: str = "COM5") -> Optional[str]:
    """
    Find the VCU's serial port, with retry logic.

    Args:
        timeout_s: Max time to search for device
        default_port: Default port to try (e.g., 'COM5')

    Returns:
        COM port name (e.g., 'COM5') or None if not found
    """
    # Try default port first
    ports = [p.device for p in serial.tools.list_ports.comports()]
    if default_port in ports:
        print(f"[OK] Found VCU on {default_port}")
        return default_port

    # Search for STM32/CDC devices
    start = time.time()
    while time.time() - start < timeout_s:
        ports = serial.tools.list_ports.comports()
        for port in ports:
            desc = (port.description or "").upper()
            # Look for STM32, CDC, or USB Serial devices
            if any(x in desc for x in ["STM32", "CDC", "USB SERIAL"]):
                print(f"[OK] Found VCU on {port.device}: {port.description}")
                return port.device
        time.sleep(0.5)

    print(f"[FAIL] VCU not found after {timeout_s}s")
    return None


def run_tests(port: str, verbose: bool = True) -> int:
    """
    Run pytest suite against the VCU.

    Args:
        port: Serial port (e.g., 'COM5')
        verbose: Print verbose output

    Returns:
        Exit code from pytest
    """
    cmd = [
        sys.executable, "-m", "pytest",
        "test_fsm.py",
        f"--port={port}",
        "-v" if verbose else "-q",
        "--tb=short",
    ]

    print(f"\n{'='*60}")
    print(f"Running: {' '.join(cmd)}")
    print(f"{'='*60}\n")

    result = subprocess.run(cmd, timeout=120)
    return result.returncode


def main():
    """Main entry point."""
    print("HIL Test Runner for GitHub Actions")
    print("=" * 60)

    # Find device
    port = find_vcu_port(timeout_s=30)
    if not port:
        print("\n[FAIL] VCU device not found")
        return 1

    # Run tests
    try:
        exit_code = run_tests(port, verbose=True)
        if exit_code == 0:
            print("\n[OK] All tests PASSED")
        else:
            print(f"\n[FAIL] Tests FAILED (exit code: {exit_code})")
        return exit_code
    except subprocess.TimeoutExpired:
        print("\n[FAIL] TIMEOUT: Tests hung (>120s)")
        return 2
    except Exception as e:
        print(f"\n[FAIL] ERROR: {e}")
        return 3


if __name__ == "__main__":
    sys.exit(main())
