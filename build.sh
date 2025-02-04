#!/bin/sh

set -xe

picotool reboot -f -u

cmake --build build

picotool load -F -x ./build/st7789v_drv.elf