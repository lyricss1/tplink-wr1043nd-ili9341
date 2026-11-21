#!/bin/sh

#load script v1.4

MODULE="/root/driver/spidevnew.ko"
MODULE_NAME="spidevnew"
DRIVER="/sys/bus/platform/drivers/leds-gpio"

echo "Starting module..."

if ! lsmod | grep -q "^${MODULE_NAME} "; then
    insmod "$MODULE" 2>/dev/null
fi
#double check
if ! lsmod | grep -q "^${MODULE_NAME} "; then
    echo "First failed, retrying"
    sleep 1
    insmod "$MODULE" 2>/dev/null
fi

if ! lsmod | grep -q "^${MODULE_NAME} "; then
    echo "!Failed load $MODULE_NAME!"
    exit 1
fi

echo "SUCCESS! Module loaded"
echo "Freeing GPIO..."

D=$(ls "$DRIVER" | grep -v -E '^(bind|unbind|uevent|module)$')
if [ -n "$D" ]; then
    echo "$D" > "$DRIVER/unbind" || {
        echo "!Cannot unbind LEDs driver"
        exit 1
    }
fi

if [ ! -c /dev/spidev0.1 ]; then
    echo "ERROR: /dev/spidev0.1 not found!"
    exit 1
fi

if [ ! -d /sys/class/gpio/gpio12 ]; then
    echo 12 > /sys/class/gpio/export 2>/dev/null
fi

if [ ! -d /sys/class/gpio/gpio12 ]; then
    echo "ERROR: GPIO12 is not available!"
    exit 1
fi

echo out > /sys/class/gpio/gpio12/direction
echo 1 > /sys/class/gpio/gpio12/value

echo "Done."

