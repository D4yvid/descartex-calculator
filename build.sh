#!/bin/sh

# Enable tracing and strict mode
set -xeuo pipefail

picotool reboot -f -u

cmake --build build

picotool load -F -x ./build/
