# NES Input Module — Plan

**Status:** Planning — no code yet  
**Module:** `firmware/modules/input-nes-snes/`  
**Target MCU:** Raspberry Pi RP2350B (Pico SDK, C++)  
**Native protocol:** NES/SNES 5 V latched serial  
**Link to core:** SPI subnode (TASD-packetized)

## 1. Purpose

Provide a self-contained firmware image for a docked RP2350B input module that:

- Reads a NES or SNES controller via the 5 V latched serial bus with proper level shifting
- Converts controller state into TASD packets
- Sends TASD packets over SPI to the CHIRBot core as a subnode
- Supports standard NES/SNES controllers with one controller per module

This module is one of the first vertical slices for CHIRBot. The NES/SNES protocol is well understood and shares the same electrical translation approach as the prototype SNES path. NES and SNES use the same electrical protocol with different physical pinouts; the first version treats them identically.

## 2. Hardware context

### 2.1 Controller bus

- NES/SNES: 8-bit or 16-bit serial, controller drives DATA push-pull, console drives LATCH and CLOCK
- Voltage: 5 V on controller side, 3.3 V logic on RP2350B
- Level shifting required: 5 V → 3.3 V on DATA input, 3.3 V → 5 V on LATCH/CLOCK outputs
- Use fixed-direction translators (e.g., SN74LVC125 for 5 V→3.3 V, SN74AHCT125 for 3.3 V→5 V) as per prototype
- First version supports standard controllers only, one controller per module, no auto-detect needed

### 2.2 Module docking

- Board-to-board docking to core via 2.54 mm shrouded header pair
- SPI signals: SCLK, MOSI, MISO, CS, IRQ/ready
- Power: 5 V from core, local 3.3 V regulator
- No link cable; module PCB outline and connector placement defined in `docs/specs/module-interface.md`

### 2.3 RP2350B resources

- 48 GPIO, 12 PIO state machines
- PIO used for precise LATCH/CLOCK timing and DATA sampling
- SPI subnode role: core drives clock, module responds
- External QSPI flash for firmware

## 3. Firmware architecture

### 3.1 Project layout

```
input-nes-snes/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── nes_protocol.cpp / nes_protocol.hpp
│   ├── spi_subnode.cpp / spi_subnode.hpp
│   ├── tasd_wrapper.cpp / tasd_wrapper.hpp
│   └── config.cpp
├── include/
└── README.md
```

### 3.2 Responsibilities

- **nes_protocol**: PIO state machine(s) to generate LATCH pulses and CLOCK edges, sample DATA
- **spi_subnode**: Implement SPI matrix subnode protocol per `docs/specs/spi-matrix-protocol.md`
- **tasd_wrapper**: Map controller state to TASD packets using `chirbot-common` TASD conventions
- **config**: Module identity, polling rate

### 3.3 Timing

- Output device owns poll edge per `docs/specs/clock-domains.md`
- Input module samples at ~1 ms, forwards latest state to core
- PIO ensures accurate LATCH/CLOCK timing independent of CPU load

## 4. TASD mapping

- Controller state: 8 or 16 bits depending on controller type
- TASD packet type: input event with timestamp, controller ID, button bitmask
- Use `chirbot-common` TASD wrappers; do not reimplement serialization
- Packet size ≤ 64 bytes per poll
- First version uses a fixed mapping for standard controllers

## 5. Development steps

1. Create Pico SDK project skeleton for RP2350B
2. Implement PIO NES/SNES reader
3. Implement SPI subnode handshake and packet framing
4. Integrate TASD wrapper
5. Add config and module identity
6. Unit test with logic analyzer
7. Integration test with core and SNES console

## 6. Testing

- Verify level shifting with scope
- Verify PIO timing against NES/SNES spec
- Verify SPI packet integrity with core
- Record and replay TASD data
- Measure latency < 1 ms end-to-end

## 7. Open questions

- Support for Super Scope/mouse variants?
- Multiple controllers per module?
- Power budget for level shifters?
- Future enhancements: auto-detect, variant support

## 8. References

- `docs/ARCHITECTURE.md` §2.3
- `docs/specs/spi-matrix-protocol.md`
- `docs/specs/module-interface.md`
- `docs/specs/tasd-usage.md`
- `docs/specs/clock-domains.md`
- `hardware/PROTOTYPE.md` — SNES translation details
- `firmware/modules/README.md`
