# HIL Testing

The VCU exposes a binary command protocol over USB CDC. Python tests drive the FSM by sending commands and asserting on responses. Tests run automatically in CI via a self-hosted GitHub Actions runner attached to the board.

## Workflows

ci.yml runs Python unit tests on ubuntu-latest on every PR. No hardware required.

hil-tests.yml runs on firmware or test file changes. A build job compiles the firmware on ubuntu-latest and uploads the .bin as an artifact. The hil-tests job runs on the self-hosted runner, downloads the artifact, flashes the board, and runs pytest.

## Protocol

Commands are a 2-byte header [CMD, LEN] followed by LEN payload bytes. The response byte is CMD | 0x80. STM32 log lines ([timestamp] ...) are automatically stripped from binary responses and routed to pytest log capture.

## Runner Setup

The self-hosted runner must have the VCU connected via both USB and SWD.

Register the runner under Settings, Actions, Runners and add the label `stm32f407` during setup.

Install STM32CubeProgrammer from ST and make sure `STM32_Programmer_CLI` is on the runner
PATH. The flash step uses SWD; USB alone is not sufficient for flashing.

### Linux

Install the udev rule to give the board a stable port at /dev/vcu:

    sudo cp 99-vcu.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules && sudo udevadm trigger

Verify with `ls -l /dev/vcu`. If the symlink does not appear, check the VID/PID with
`lsusb` and update the rule to match.

### Windows

Install Python from python.org.

Add the STM32CubeProgrammer `bin` directory to the system PATH. The default path is:

    C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin

The workflow relies on the board enumerating as COM5 (the default used by the test suite
on Windows). To pin the board to COM5, open Device Manager, find the board under Ports
(COM & LPT), go to Properties → Port Settings → Advanced and set the COM port number.

If COM5 is not available, set the `VCU_PORT` environment variable on the runner to the
correct port (e.g. `COM3`). The test suite reads it automatically.

## Running Locally

Linux:

    pytest tools/tests/test_fsm.py --port /dev/vcu -v

Windows:

    pytest tools/tests/test_fsm.py --port COM5 -v

`VCU_PORT` env var can be set instead of passing `--port` each time.
