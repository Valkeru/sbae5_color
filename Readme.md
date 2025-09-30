# Creative SoundBlaster AE5 and AE5Plus LED driver for Linux

# WIP

## Build

### Dependencies

To build package you need dkms installed  

# Debian-based
Use docker:  
```shell
docker compose build
docker compose run --rm builder
sudo apt install ./sbae5-color-driver-dkms_0.2_all.deb
```

Next install deb file

### Arch
```shell
$ sudo pacman -Syu dkms linux-headers
$ makepkg
$ sudo pacman -U sbae5-color-dkms-0.2-1-any.pkg.tar.zst
```

## How to use

To get particular device name and PCI bus location you can read following files:  
`/sys/kernel/sbae5_color/device_name`  
`/sys/kernel/sbae5_color/device_location`

To set LED colors, you need to put 16 bytes sequence into `/proc/sbae5_color` file:  
+ byte 1 - LED count to change, 1 to 5
+ bytes 2-6 - red values for LEDs
+ bytes 7-11 - green values for LEDs
+ bytes 12-16 - blue values for LEDs

For C/C++ just write structure described in [led_data.h](led_data.h)

Bash:

```shell
#! /usr/bin/env bash

PROC_FILE="/proc/sbae5_color"
MAX_LEDS=5

if [[ ! -w "$PROC_FILE" ]]; then
    echo "Unable to write $PROC_FILE"
    exit 1
fi

RED_COLORS="ffff0000ff"
GREEN_COLORS="00ffffff00"
BLUE_COLORS="000000ffff"
LED_COUNT="05"

DATA="${LED_COUNT}${RED_COLORS}${GREEN_COLORS}${BLUE_COLORS}"

echo "$DATA" | xxd -r -p > "$PROC_FILE"
```