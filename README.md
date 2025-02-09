# Descartex Scientific Calculator

This project is a scientific calculator written (almost) from scratch in C, for the
`Raspberry Pi Pico (RP2040)` board, equipped with an `Sitronix ST7789V` controlled display.

### State
This project is in early development, i'm building the display driver for now, so, when this is
more advanced, i'll put pictures of the Pico running this in baremetal.

### Building

- **MacOS X**:
  First, install the ARM Embedded Toolchain from the ARM website, for the triplet `arm-none-eabi`.

  To build, first, source the environment for mac and run the build script:
```sh
descartex-calculator $ source env/macos.sh
descartex-calculator $ ./build.sh
```

- **Windows**:
  Install VSCode and the Raspberry Pi Pico extension, and import this project. It will setup the
  SDK automatically.

  To build, just run the `build.bat` file inside VSCode terminal:
```powershell
# For Powershell
 PS descartex-calculator> .\build.bat

# For Command Prompt
CMD descartex-calculator> build
```
