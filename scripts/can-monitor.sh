#!/bin/bash
# Brings up the CANable 2.0 as a SocketCAN interface and runs candump.
# Identifies the device by USB vendor (Openlight Labs) so ttyACM number shifts don't matter.

BITRATE_FLAG="-s6"  # 500 kbps — change to -s5 (250k) or -s8 (1M) if needed
IFACE="can0"

find_canable() {
    for dev in /dev/ttyACM*; do
        if udevadm info "$dev" 2>/dev/null | grep -q "ID_VENDOR=Openlight_Labs"; then
            echo "$dev"
            return
        fi
    done
}

DEV=$(find_canable)

if [ -z "$DEV" ]; then
    echo "Error: CANable not found. Is it plugged in?" >&2
    exit 1
fi

echo "Found CANable at $DEV"

if ! ip link show "$IFACE" &>/dev/null; then
    echo "Starting slcand on $DEV..."
    sudo slcand -o -c -f "$BITRATE_FLAG" "$DEV" "$IFACE"
    sudo ip link set "$IFACE" up
else
    echo "$IFACE already up"
fi

echo "Dumping CAN bus (Ctrl+C to stop):"
candump -t a "$IFACE"
