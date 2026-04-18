# Pico CMake Build Skill

**Domain:** Raspberry Pi Pico SDK projects using CMake  
**Audience:** Platform developers, firmware builders  
**Status:** Stable

## Overview

Pico SDK projects follow a standard CMake + Ninja workflow. This skill documents the expected patterns and how to verify build correctness.

## Expected Structure

```
project-root/
├── CMakeLists.txt           # Must include pico_sdk_import.cmake and pico_sdk_init()
├── pico_sdk_import.cmake    # Standard copy from Pico SDK
├── main.cpp (or .c)         # Application entry point
├── *.pio                     # PIO programs (if using PIO)
└── build/                    # Generated (not in version control)
    ├── CMakeCache.txt
    ├── build.ninja
    ├── generated/            # Generated headers from CMake macros
    ├── *.elf, *.hex, *.uf2   # Final firmware artifacts
    └── CMakeFiles/           # Object files
```

## Build Command Pattern

For any Pico project:

```bash
# First time (or after CMakeLists.txt change)
mkdir -p build && cd build
cmake -G Ninja ..

# Build
cmake --build .
```

Or with explicit configuration:
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

## Key CMake Macros (Pico SDK)

| Macro | Purpose | Example |
|-------|---------|---------|
| `pico_sdk_init()` | Initialize SDK (must be after `project()`) | `pico_sdk_init()` |
| `add_executable()` | Create firmware target | `add_executable(myapp main.cpp)` |
| `pico_add_extra_outputs()` | Generate .hex, .uf2, .bin from .elf | `pico_add_extra_outputs(myapp)` |
| `pico_generate_pio_header()` | Compile .pio → .pio.h | `pico_generate_pio_header(myapp myprogram.pio)` |
| `target_link_libraries()` | Link against Pico libraries | `target_link_libraries(myapp pico_stdlib hardware_pio)` |

## Generated Artifacts

After successful `cmake --build .`:

| File | Format | Use |
|------|--------|-----|
| `*.elf` | ELF executable | Debugging, symbol lookup |
| `*.uf2` | UF2 bootloader format | Drag-and-drop to BOOTSEL mode |
| `*.hex` | Intel HEX | Programmer tools |
| `*.bin` | Raw binary | Direct flashing |
| `*.pio.h` | Generated C header | Included by application |

## PIO Header Generation

**Pattern:** For each `.pio` file, a corresponding `.pio.h` is **auto-generated at build time**.

```cmake
# In CMakeLists.txt
pico_generate_pio_header(myapp ${CMAKE_CURRENT_LIST_DIR}/myprogram.pio)
```

**Result:** `build/myprogram.pio.h` is created and can be included in source:
```cpp
#include "myprogram.pio.h"
```

**Do not commit .pio.h files to version control** — they are derived artifacts.

## Detecting Build Issues

**No tests?** → Check for:
- Compilation errors (linker, missing symbols)
- `.pio` files with syntax errors (pioasm will fail silently in some IDEs)
- Missing `target_link_libraries()` for required hardware

**No linting?** → Expect:
- Code style varies by contributor
- No automated formatting
- Manual review required

## SDK Locator Logic

Pico SDK location resolved in order:
1. `PICO_SDK_PATH` environment variable
2. `pico_sdk_import.cmake` cache (if CMake has been run)
3. Pico VS Code extension location (`~/.pico-sdk/`)
4. Git clone from `https://github.com/raspberrypi/pico-sdk` (if `PICO_SDK_FETCH_FROM_GIT=ON`)

**Best practice:** Let VS Code extension manage the SDK; don't set env vars unless troubleshooting.

## Troubleshooting

| Error | Cause | Fix |
|-------|-------|-----|
| `pioasm not found` | SDK not located | Check `PICO_SDK_PATH` or reinstall VS Code extension |
| `undefined reference to hardware_*` | Missing `target_link_libraries()` | Add library to CMakeLists.txt |
| `blink.pio.h: file not found` | PIO header not generated | Run `cmake --build .` (don't skip generation step) |
| `PICO_BOARD not set` | Board not specified | Add `set(PICO_BOARD board_name)` before `pico_sdk_init()` |

