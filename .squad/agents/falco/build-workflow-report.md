# CHIRBot Build Workflow Report

**Reported by:** Falco (Platform Dev)  
**Date:** 2026-04-18  
**Status:** Investigation Complete

---

## Build Command

The CHIRBot project uses **CMake** with **Ninja** as the build system generator.

**Build command:**
```bash
cd C:\ws\CHIRBot\build
cmake --build . [--config Release]
```

Or direct Ninja invocation:
```bash
cd C:\ws\CHIRBot\build
ninja
```

**Full from-scratch build:**
```bash
cd C:\ws\CHIRBot
rm -r build
mkdir build
cd build
cmake -G Ninja ..
cmake --build .
```

---

## Test & Lint Commands

**FINDING:** There are **NO test or lint commands** present in this repository.
- No `tests/` directory exists
- No pytest/unittest/CTest configuration
- No linter tools configured (clang-format, cppcheck, etc.)
- No CI workflows that run tests (GitHub Actions only has squad-triage and heartbeat, not build/test)

**Implication:** This is a **firmware project with no automated testing infrastructure**. All validation is manual or device-level.

---

## Generated Artifact Flow: `blink.pio.h`

### How it enters the build:

1. **CMakeLists.txt (line 44):**
   ```cmake
   pico_generate_pio_header(CHIRBot ${CMAKE_CURRENT_LIST_DIR}/blink.pio)
   ```
   This macro (from Pico SDK) invokes `pioasm` to compile `blink.pio` into a C header.

2. **Input file:** `C:\ws\CHIRBot\blink.pio`  
   Contains the PIO assembly program and C helper function template (`blink_program_init`).

3. **Generated output:** `C:\ws\CHIRBot\build\blink.pio.h`  
   Created at **build time** (not checked into version control).

4. **Consumed by:** `main.cpp` (line 33)
   ```cpp
   #include "blink.pio.h"
   ```

5. **Build dependency chain:**
   - `CHIRBot_blink_pio_h` (phony target in build.ninja)
   - Depends on: `blink.pio` source file
   - Prerequisite for: `CMakeFiles/CHIRBot.dir/main.cpp.obj` compilation

### Build artifact outputs:

The CMake configuration (`pico_add_extra_outputs` on line 71) generates multiple binary formats:
- `CHIRBot.elf` — ELF executable (primary firmware image)
- `CHIRBot.hex` — Intel HEX format
- `CHIRBot.bin` — Raw binary
- `CHIRBot.uf2` — UF2 bootloader format (for drag-and-drop programming to Pico)

**Location:** All generated in `C:\ws\CHIRBot\build/` after successful build.

---

## Pico SDK Assumptions

### Detected Configuration:

| Setting | Value |
|---------|-------|
| **Board** | `pimoroni_pico_plus2_w_rp2350` (line 26 of CMakeLists.txt) |
| **SDK Version** | `2.2.0` (from CMakeLists.txt line 18) |
| **Toolchain Version** | `14_2_Rel1` (Arm GCC 14.2) |
| **picotool Version** | `2.2.0-a4` |
| **CMAKE Version** | 3.31.5 (from `.pico-sdk/cmake/v3.31.5/`) |
| **C Standard** | C11 |
| **CXX Standard** | C++17 |

### SDK Integration:

- **Location:** Managed by Pico VS Code extension bootstrap (lines 13–24)
  - Reads `pico_sdk_import.cmake` (standard Pico SDK import)
  - Checks for `~/.pico-sdk/cmake/pico-vscode.cmake` (VS Code extension)
  - Falls back to `PICO_SDK_PATH` environment variable or Git fetch

- **Hardware Libraries Linked:**
  - `hardware_spi`, `hardware_i2c`, `hardware_dma`, `hardware_pio`
  - `hardware_interp`, `hardware_timer`, `hardware_watchdog`, `hardware_clocks`
  - `pico_stdlib` (core runtime)

- **I/O Configuration:**
  - UART output enabled (`pico_enable_stdio_uart` = 1, line 47)
  - USB output disabled (`pico_enable_stdio_usb` = 0, line 48)

### Board-Specific Assumptions:

The `pimoroni_pico_plus2_w_rp2350` board:
- Uses **RP2350** dual-core ARM Cortex-M33 MCU
- WiFi variant (CYW43) implied by board name (`_w_`)
- See: `C:\Users\thegu\.pico-sdk\sdk\2.2.0\src\boards\` for board definitions

---

## No Single-Test Equivalent

**Status:** N/A — No unit tests exist.

This is a bare-metal firmware project. Validation is:
- **Compile check:** Does `cmake --build .` succeed?
- **Link check:** Do all symbols resolve?
- **Device check:** Flash to board and observe LED blinking, UART output

---

## Build Invocation Summary

**Documented commands that ACTUALLY EXIST in this repo:**

| Command | Purpose | Works? |
|---------|---------|--------|
| `cmake -G Ninja ..` (from clean build/) | Generate Ninja build files | Yes (CMakeLists.txt present) |
| `cmake --build .` | Build project | Yes (executes ninja) |
| `ninja -C build` | Build with Ninja directly | Yes (build.ninja exists) |
| `cmake -G "Unix Makefiles" ..` | Alternative (Makefile) | Yes (CMake supports it, untested) |

**Commands NOT found (do not invent):**
- `make` — No Makefile template
- `pytest` / `unittest` — No test framework
- `clang-format`, `cppcheck` — No linting configured
- `ctest` — No CTest configuration

---

## Key Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Main build configuration |
| `pico_sdk_import.cmake` | Pico SDK locator (standard copy) |
| `main.cpp` | Application entry point |
| `blink.pio` | PIO assembly program for LED blinking |
| `build/build.ninja` | Generated Ninja build graph |
| `build/CMakeCache.txt` | CMake configuration cache |

