# Decisions

## 2026-04-18

### Squad Initialization
- Squad initialized for CHIRBot using the Star Fox naming set.
- This repository is treated as an embedded firmware project built with CMake and the Raspberry Pi Pico SDK.
- Copilot guidance should stay focused on repository-specific build flow, firmware architecture, and project conventions.

### Copilot Instructions (Slippy)
Created `.github/copilot-instructions.md` to establish clear, repository-specific guidance for Copilot sessions.
- **Hardware**: RP2350B (Pimoroni Pico Plus 2 W), dual Cortex-M33 @ 150 MHz
- **Peripherals**: SPI0 (1 MHz), I2C0 (400 kHz), DMA, PIO (LED blinking), UART1 (115200), Watchdog (100 ms), Timers, Clock monitoring
- **Code Conventions**: Naming patterns, include order (std→Pico→hardware→generated), hardware init pattern (module init→GPIO function→GPIO dir→options)
- **Integration Boundaries**: Falco owns board/SDK pinning; Slippy owns firmware logic; escalate via `.squad/decisions.md`
- **Build**: CMake with Ninja, no formal test suite, manual device-level validation

### Build Workflow (Falco)
Confirmed build pipeline and Pico SDK assumptions.
- **Command**: Shell-agnostic sequence: `cmake -S . -B build -G Ninja` then `cmake --build build`
- **PIO Flow**: `blink.pio` → CMake `pico_generate_pio_header()` → `build/blink.pio.h` → `main.cpp`
- **SDK Lock**: Board=`pimoroni_pico_plus2_w_rp2350`, SDK=2.2.0, ARM GCC 14.2
- **No Test/Lint**: Bare-metal firmware with compile + device-level validation only
- **Generated Artifacts**: `CHIRBot.{elf,hex,bin,uf2}` to `build/` directory
- **Watchdog Hazard**: Firmware enables 100 ms watchdog, updates once at startup, then loops with 1000 ms sleep—will reboot unless watchdog is fed in loop or disabled.

### Validation Guidance (Peppy)
Reviewed testing and validation strategy.
- **Automated Testing**: None (bare-metal firmware architecture)
- **Manual Validation Points**: 
  - UART serial output monitoring during init
  - LED blink observation (3 Hz frequency via PIO)
  - Watchdog refresh cycle verification
  - DMA string copy correctness
- **Toolchain**: Ninja generator (configured in `.vscode/settings.json`), CMake/Ninja pre-installed via Pico SDK extension
- **VS Code Integration**: Pre-configured tasks for build, flash (picotool/OpenOCD), and debug

### Architecture Stack Decisions (Approved)
- **USB HID Stack**: TinyUSB (approved)
- **MicroSD Library**: Custom TASD writer (deferred to implementation phase)
- **Recording Trigger**: Menu system on display
- **Clock Source**: Pico core timer (not host-provided)
- **Watchdog Behavior**: Graceful failure (not hard reboot)

### Future Recommendations (Pending)
- **Short-term**: Update Copilot instructions to clarify Ninja generator and pre-installed toolchain
- **Medium-term**: Add Makefile with `build/clean/rebuild` targets; Python script for UART output automation
- **Long-term**: Introduce unit test framework or clang-tidy if project scope expands beyond firmware demo
