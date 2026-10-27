D=$(ls /sys/bus/platform/drivers/leds-gpio | grep -v -E '^(bind|unbind|uevent|module)$')
echo "$D" > /sys/bus/platform/drivers/leds-gpio/unbind
rmmod spi-gpio-custom 2>/dev/null
insmod /lib/modules/4.14.275/spi-gpio-custom.ko bus0=1,19,18,4294967295,0,8000000,15
ls -l /dev/spidev1.0
