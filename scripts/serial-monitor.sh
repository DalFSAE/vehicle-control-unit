#!/bin/bash
# Monitors the VCU USB CDC serial output with auto-reconnect.
# Identifies the device by model (STM32 Virtual ComPort) so ttyACM number shifts don't matter.

BAUD=115200

find_vcu_serial() {
    for dev in /dev/ttyACM*; do
        if udevadm info "$dev" 2>/dev/null | grep -q "ID_MODEL=STM32_Virtual_ComPort"; then
            echo "$dev"
            return
        fi
    done
}

echo "Waiting for VCU serial port (Ctrl+C to stop)..."

while true; do
    DEV=$(find_vcu_serial)

    if [ -z "$DEV" ]; then
        echo "VCU not found, retrying..."
        sleep 1
        continue
    fi

    echo "Connected to $DEV at ${BAUD} baud"
    picocom -b "$BAUD" "$DEV"
    echo "Disconnected, reconnecting..."
    sleep 1
done
