# Peppy History

## Learnings
- Project requested by Fortinbra.
- Validation guidance must reflect the actual repository commands and existing build outputs.
- **2026-04-18 Validation Review**: CHIRBot uses CMake + Pico SDK with Ninja as the build generator (not Unix Makefiles). No formal test suite, linting, or CI/CD exists—validation is manual (UART output, LED blink, watchdog). Copilot instructions are accurate but should clarify Ninja generator and that CMake/toolchain are pre-installed via Pico SDK extension. VS Code tasks.json provides three flashing paths: picotool (UF2), OpenOCD (CMSIS-DAP), and rescue reset. Core validation points: serial output during init, LED blink at 3 Hz, watchdog refresh cycle, DMA string copy.
- Key file paths: `main.cpp` (C++17, baremetal), `CMakeLists.txt` (SDK config), `.vscode/tasks.json` (build tasks), `.vscode/settings.json` (Ninja/CMake paths), `compile_commands.json` (IDE support).

## Cross-Agent Context (2026-04-18)
- **Slippy (Firmware)**: Documented hardware init pattern (module init → GPIO function → GPIO dir → options) and all active peripherals with pinouts. Copilot instructions now provide concrete naming conventions and architectural guidance for future firmware changes.
- **Falco (Platform)**: Confirmed Ninja + CMake build flow, no test/lint infrastructure. Generated PIO header must not be checked in. SDK environment managed by VS Code extension at `~/.pico-sdk/`; no standalone toolchain installation required.
- **Future Automation**: Potential for Python script to monitor UART output; Makefile with common targets (`build`, `clean`, `rebuild`); GitHub Actions CI for multi-platform builds.
