# Scripts

## Shell

- `can-monitor.sh` — Monitor CAN traffic via USB adapter
- `serial-monitor.sh` — Monitor VCU serial output

## HIL Testing (`hil/`)

Hardware-in-the-loop testing over USB CDC.

```bash
cd hil
pip install -r requirements.txt
pytest test_fsm.py --port /dev/ttyACM0 -v
```

Uses `vcu_hil.py` for serial transport; `test_fsm.py` for FSM state transitions and fault injection.

Runs automatically on self-hosted runner in `.github/workflows/hil.yml`.
