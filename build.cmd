@echo off

ECHO [INFO] Rebooting RPI Pico into BOOTSEL mode
REM Restart the Pico to BOOTSEL mode
picotool reboot -f -u >nul

ECHO [INFO] Running 'cmake --build .\build\'
REM Build the ELF image
cmake --build .\build\

ECHO [INFO] Loading new firmware into RPI Pico
REM Flash the ELF image into the Pico
:loop
picotool load -F -x .\build\descartex.elf >nul || (
    GOTO loop
)

ECHO [INFO] Done.
