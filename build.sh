#!/bin/sh

# Enable tracing and strict mode
set -xeuo pipefail

# Reboot target device using picotool
picotool reboot -f -u

# Build project
cmake --build build

# Load the compiled file into the Pico
while ! picotool load -F -x ./build/descartex.elf 2> /dev/null; do
	wait
done
