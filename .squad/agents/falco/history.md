# Falco History

## Learnings
- Project requested by Fortinbra.
- The Pico VS Code extension bootstrap is embedded in `CMakeLists.txt`.
- The target board is `pimoroni_pico_plus2_w_rp2350`.
- **Build system:** CMake + Ninja generator. Command: `cmake -G Ninja ..` then `cmake --build .` from `build/` directory.
- **No tests/linting:** This is a bare-metal firmware project with no test framework, unit tests, or linter setup. Validation is compile + device-level only.
- **PIO artifact flow:** `blink.pio` → pioasm (via `pico_generate_pio_header()` macro) → `build/blink.pio.h` → included in `main.cpp` line 33.
- **Generated binaries:** `CHIRBot.elf`, `.hex`, `.bin`, `.uf2` all output to `build/` directory via `pico_add_extra_outputs()`.
- **Pico SDK:** Version 2.2.0, ARM GCC 14.2 toolchain, located in `~/.pico-sdk/` (managed by VS Code extension).
- **Hardware libraries:** Linked against hardware_spi, hardware_i2c, hardware_dma, hardware_pio, hardware_interp, hardware_timer, hardware_watchdog, hardware_clocks + pico_stdlib.

## Cross-Agent Context (2026-04-18)
- **Slippy (Firmware)**: Created `.github/copilot-instructions.md` with hardware architecture (RP2350B, peripherals, pinouts), code conventions (naming, include order, init pattern), and team boundaries. Copilot guidance now focused on firmware-specific logic and peripheral setup.
- **Peppy (Tester)**: Validation approach is honest and appropriate—no formal test suite exists. CMake/Ninja toolchain pre-installed via VS Code extension; users don't need standalone installations. Manual validation via UART output, LED observation, watchdog cycles, DMA verification sufficient for firmware scope.
