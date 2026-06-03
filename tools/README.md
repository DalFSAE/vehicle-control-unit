# HIL Testing

The VCU exposes a binary command protocol over USB CDC. Python tests drive the FSM by sending commands and asserting on responses. Tests run automatically in CI via a self-hosted GitHub Actions runner attached to the board.

## Workflows

ci.yml runs Python unit tests on ubuntu-latest on every PR. No hardware required.

hil-tests.yml runs on firmware or test file changes. A build job compiles the firmware on ubuntu-latest and uploads the .bin as an artifact. The hil-tests job runs on the self-hosted runner, downloads the artifact, flashes the board, and runs pytest.

## Protocol

Commands are a 2-byte header [CMD, LEN] followed by LEN payload bytes. The response byte is CMD | 0x80. STM32 log lines ([timestamp] ...) are automatically stripped from binary responses and routed to pytest log capture.

## Runner Setup

The self-hosted runner must be a Linux machine with the VCU connected via both USB and SWD.

Register the runner under Settings, Actions, Runners and add the label stm32f407 during setup.

Install the udev rule to give the board a stable port at /dev/vcu:

    sudo cp 99-vcu.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules && sudo udevadm trigger

Verify with ls -l /dev/vcu. If the symlink does not appear, check the VID/PID with lsusb and update the rule to match.

Install STM32CubeProgrammer from ST and make sure STM32_Programmer_CLI is on the runner PATH. The flash step uses SWD, USB alone is not sufficient for flashing.

## Running Locally

    pytest test_fsm.py --port /dev/vcu -v

VCU_PORT env var can be set instead of passing --port each time.
