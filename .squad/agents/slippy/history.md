# Slippy History

## Learnings
- Project requested by Fortinbra.
- CHIRBot firmware is built with CMake and the Raspberry Pi Pico SDK.
- The current code centers on `main.cpp` plus the generated PIO header from `blink.pio`.
- **Board Target**: Pimoroni Pico Plus 2 W with RP2350B MCU (dual Cortex-M33, 150 MHz).
- **Peripherals in Use**: SPI0 (1 MHz, GPIO16-19), I2C0 (400 kHz, GPIO8-9), DMA, PIO (LED blinking), UART1 (115200, GPIO4-5), Watchdog (100 ms), Timers, Clock monitoring, Interpolator (unused).
- **Main Runtime**: Initializes peripherals, starts PIO LED task, enters infinite loop with watchdog feeding and 1 second print cycles.
- **PIO Usage**: `blink_program` in `blink.pio` toggles GPIO at programmable frequency; auto-generated to `blink.pio.h` by CMake.
- **Compile Configuration**: C11/C++17 standards, export_compile_commands enabled, UART stdio enabled, USB stdio disabled.
- **No formal test suite** exists; validation is manual (serial output monitoring, LED observation, watchdog behavior, DMA test string).
- **Critical Pattern**: All peripheral inits follow: (1) module init, (2) GPIO function select, (3) GPIO direction/state, (4) options (pull-ups, etc.).

## Cross-Agent Context (2026-04-18)
- **Falco (Platform)**: Build confirmed via `cmake -G Ninja && cmake --build .` from `build/` directory. SDK v2.2.0, ARM GCC 14.2, located in `~/.pico-sdk/`. No test/lint infrastructure (bare-metal firmware). Generated PIO header must NOT be checked in.
- **Peppy (Tester)**: Manual validation strategy is appropriate for firmware scope. Toolchain (CMake/Ninja) pre-installed via VS Code Pico SDK extension; users don't install separately. Validation points: UART output, LED blink at 3 Hz, watchdog refresh, DMA correctness.

