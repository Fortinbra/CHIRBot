# CHIRBot Architecture Analysis & Proposed Vertical Slices

*Fox (Lead), Architecture Owner*  
*Analysis of current firmware state and roadmap for modular input relay*

---

## Current Firmware Analysis

### What We Have (Demo Sketch: main.cpp)

The current `main.cpp` is a **multi-peripheral demo sketch** showcasing Raspberry Pi Pico capabilities:
- **SPI Master** (1 MHz) — GPIO 16–19, ready for external slave communication
- **I2C** (400 kHz) — GPIO 8–9, configured but unused
- **DMA** — Example copying string buffer (proof of concept for throughput)
- **PIO** — Blinking LED with frequency control (GPIO 6 or PICO_DEFAULT_LED_PIN)
- **Timer/Alarm** — Callback framework ready
- **Watchdog** — 100ms reboot guard
- **UART1** — 115200 baud on GPIO 4–5 (secondary; stdio on default uart0)
- **Clocks** — System clock frequency logging
- **Interpolator** — Loaded but not used

### Clutter vs. Core Relay Logic

| Demo Code | Status | Rationale |
|-----------|--------|-----------|
| SPI Master setup | **KEEP** | Core input communication (TASD protocol from host) |
| SPI DMA config | **KEEP** | Needed for 1ms latency relay throughput |
| I2C init | **REMOVE** | No multi-peripheral plan; core is single input/output |
| DMA copy example | **REFACTOR** | Replace with SPI RX DMA chain (not buffer-copy demo) |
| PIO blink | **OPTIONAL** | Keep for status LED, move to optional output module |
| Interpolator | **REMOVE** | Not part of core relay |
| Timer/Alarm example | **REFACTOR** | Use for input poll scheduling, not demo callback |
| Watchdog enable | **KEEP** | Critical for production (failsafe on stuck relay) |
| UART1 init | **OPTIONAL** | Use for debug logging, not part of I/O contract |

---

## Clock Domain & Latency Constraints

### SPI Communication (Input Path)
- **Clock:** SPI 1 MHz (configured in demo)
- **Latency requirement:** <1 ms core relay (output device drives poll edge at ~125 Hz = 8ms periods)
- **DMA throughput:** At 1 MHz, a 12-byte TASD packet (timestamp + buttons + analog_x/y) takes ~96 µs
- **Strategy:** Use SPI DMA to offload bytes; interrupt on frame complete to parse & relay

### Output Path (USB HID or next module)
- **Clock:** Host polls USB HID at ~125 Hz (typical gaming device)
- **Latency requirement:** <1 ms relay guarantee
- **Strategy:** Parsed input packet buffered in core; output module polls/blocks on ring buffer

### MicroSD Storage (Asynchronous)
- **Clock:** Independent; non-blocking with respect to input/output relay
- **Buffering:** Ring buffer (core writes parsed packets; SD task drains to disk)
- **Strategy:** Separate task/state machine; no blocking of relay path

---

## TASD Format (Reference from tasd.io)

CHIRBot uses the **TASD (Timestamped Analog/Switch Device) format**:

```
SPI Frame structure (12 bytes minimum):
  Byte 0:    Frame type (0x01 = input event, 0x02 = ACK, etc.)
  Bytes 1–4: Timestamp (uint32 little-endian, microseconds since epoch or session start)
  Bytes 5–6: Button state (uint16 bitmask, e.g., bit 0 = button 1, bit 1 = button 2, etc.)
  Bytes 7–8: Analog X (int16 little-endian, -32768 to +32767, representing ±1G or ±range)
  Bytes 9–10: Analog Y (int16 little-endian, same range)
  Byte 11:   Checksum (CRC8 or XOR of all prior bytes)
```

**Modular savings format (microSD):**
- TASD packets are variable-length; recordings use fixed 12-byte structs for simplicity
- File header: 4-byte magic ("TASD") + 4-byte version + 8-byte start_timestamp_us
- Each frame: 12-byte InputPacket struct

---

## Proposed Vertical Slices (Narrow, Independently Testable)

Each slice is designed to be **completable in one focused coding session** (spec + implementation + test).
Slices build on each other and avoid blocking cross-dependencies.

---

### Slice 1: Core SPI Input Reception & Parsing
**Title:** Receive and parse TASD input packets via SPI

**Why it's narrow:**
- Only handles SPI master reception (no output, no storage)
- Single responsibility: bytes → struct
- Can be tested on bench with a logic analyzer or test harness (mock SPI frames)

**What it does:**
- Core initializes SPI slave reception (or master loopback for test)
- DMA chain ingests 12-byte frames from SPI RX FIFO
- Interrupt handler parses bytes into `InputPacket` struct
- Logs parsed values to stdio for verification
- No relay to output device yet

**How it fits:**
- Foundation for all downstream features (storage, USB HID)
- Establishes the core data model & timing baseline
- Decoupled from output; can test input path in isolation

**Dependencies:**
- None (self-contained)

**Acceptance Criteria:**
- [ ] Pico configures SPI0 as master, 1 MHz clock
- [ ] DMA chain configured to receive 12-byte frames
- [ ] SPI receive interrupt parses frame into `InputPacket`
- [ ] Core logs timestamp, buttons, analog_x, analog_y correctly
- [ ] Loopback test: send mock frame via GPIO/bench tool, verify parsed struct
- [ ] Handles frame boundaries correctly (no off-by-one errors)
- [ ] Timing: <100 µs from SPI frame complete to interrupt return

---

### Slice 2: Output Relay to USB HID Module (Minimal)
**Title:** Create USB HID pluggable module interface and relay input to device

**Why it's narrow:**
- Establishes the module boundary (USB HID is separate from core)
- Core acts as data pump: InputPacket → USB HID interface
- USB HID module itself is a stub (no real USB stack yet; mock or TinyUSB integration TBD)

**What it does:**
- Defines the core-to-HID interface (function pointer callback or ring buffer protocol)
- Core relays each parsed InputPacket to HID module via function call
- HID module stub logs or enqueues to a ring buffer
- Timing verification: confirm <1 ms relay latency

**How it fits:**
- Completes the input→output path (end-to-end relay)
- Allows USB HID module to be developed in parallel without core changes
- Establishes contract for output device polling

**Dependencies:**
- Slice 1 (core must parse InputPacket first)

**Acceptance Criteria:**
- [ ] Core defines and exports `chirbot_relay_input(InputPacket *pkt)` function
- [ ] HID module registers a callback with core at startup
- [ ] Core calls callback synchronously upon each packet parse
- [ ] Callback receives correct packet data (no corruption)
- [ ] Relay latency measured <1 ms (logic analyzer or instrumentation)
- [ ] HID module can be swapped for a mock for testing

---

### Slice 3: MicroSD Recording Pipeline (Ring Buffer + Async Write)
**Title:** Record TASD-formatted input packets to microSD card

**Why it's narrow:**
- Recording is asynchronous; does not block relay latency
- Focuses on persistent storage, not on-device analysis
- Ring buffer decouples fast input from slow disk I/O

**What it does:**
- Core maintains a ring buffer (16–32 packet slots) for recording
- Input parser writes parsed packet to ring buffer (non-blocking)
- Separate task/main loop wakes periodically to flush ring to microSD file
- File format: TASD header (magic + version + timestamp) + stream of 12-byte packets
- Start/stop recording via SPI command or GPIO (TBD, leave as open question)

**How it fits:**
- Enables offline replay for testing & debugging
- Serves as fallback storage if USB connection lost
- Foundation for future on-device analysis or filtering

**Dependencies:**
- Slice 1 (core must parse InputPacket)
- Slice 2 (if USB HID is mandatory path; here we assume optional)

**Acceptance Criteria:**
- [ ] Ring buffer (e.g., 32 slots) allocated and initialized
- [ ] Input parser writes to ring buffer without blocking (<10 µs overhead)
- [ ] SD task periodically flushes buffered packets to file
- [ ] File written to microSD card with TASD header (magic + version)
- [ ] File can be read back; packets bit-match originals (no corruption)
- [ ] Recording can be started/stopped without data loss
- [ ] Handles full ring buffer gracefully (overwrite oldest or pause input?) — see open questions

---

### Slice 4: Modular Core State Machine & Configuration
**Title:** Core relay state machine (init, idle, recording, error states) and config interface

**Why it's narrow:**
- Orthogonal to input/output logic; handles lifecycle & configuration
- Establishes clear state transitions (no spaghetti init)
- Configuration via SPI commands (settable parameters like recording path, poll rate, etc.)

**What it does:**
- Define `CoreState` enum: INIT, IDLE, RECORDING, ERROR, SHUTDOWN
- State machine transitions on events: SPI_COMMAND, WATCHDOG, SD_ERROR
- Core exposes config API: `set_recording_path()`, `get_status()`, `reset_core()`
- SPI command handler interprets control frames (e.g., 0x02 = start recording)
- Logging of state transitions to UART for debugging

**How it fits:**
- Enables coordinated startup/teardown across modules
- Allows future features (e.g., filtering, multi-mode) without core redesign
- Essential for reliable operation in field

**Dependencies:**
- Slice 1 (SPI reception) for command handling
- Slice 3 (recording) for state transitions

**Acceptance Criteria:**
- [ ] State machine defined and transitions correct
- [ ] SPI command 0x02 triggers start_recording (if IDLE)
- [ ] SPI command 0x03 triggers stop_recording (if RECORDING)
- [ ] Core does not accept invalid state transitions (e.g., start recording twice)
- [ ] Each state transition logged with timestamp
- [ ] Error state reached on watchdog or SD failure; logged and reported
- [ ] Status query via SPI command 0x04 returns current state & packet count

---

### Slice 5: Integration Test Harness & Bench Firmware
**Title:** Test harness for loopback validation and bench firmware for modular integration

**Why it's narrow:**
- Focuses on testability, not production feature
- Enables integration testing without real host hardware
- Proves all slices work together correctly

**What it does:**
- Create `test/` directory with loopback firmware (configurable SPI master that sends mock frames)
- Test harness simulates: normal frame, malformed frame, burst traffic
- Bench firmware includes stdio logging & timing instrumentation
- Verifies end-to-end flow: input → core → ring buffer → SD file (or USB stub)
- Simple CLI commands for manual testing (e.g., `record start`, `record stop`, `status`)

**How it fits:**
- Allows early validation before real hardware integration
- Exercises all slices together; uncovers interdependencies
- Provides reference for future module developers

**Dependencies:**
- Slice 1–4 (all prior slices must be complete)

**Acceptance Criteria:**
- [ ] Loopback firmware compiles and runs
- [ ] Sends valid TASD frame; core receives and parses correctly
- [ ] Sends malformed frame; core handles gracefully (no crash, logged)
- [ ] Burst test: 100 frames back-to-back; no drops or corruption
- [ ] Recording test: send 100 frames, verify 100 packets on SD card
- [ ] Timing: measure relay latency for burst; confirm <1 ms average
- [ ] Manual CLI test: `record start` → (send 10 frames) → `record stop` → verify file on SD

---

## Dependency Graph

```
Slice 1: SPI Input
  ├── Slice 2: USB HID Relay (depends on Slice 1)
  ├── Slice 3: SD Recording (depends on Slice 1)
  │   └── Slice 4: Core State Machine (depends on Slice 1 & 3)
  └── Slice 5: Test Harness (depends on Slices 1–4, no blockers for others)
```

**Parallelizable pairs:**
- Slice 2 & Slice 3 (both depend only on Slice 1; can be worked in parallel)
- Slice 4 (can begin once Slice 1 is spec'd, even if not fully tested)
- Slice 5 is the final integration step

---

## Recommended Implementation Order

1. **Slice 1** ✓ (foundation; unblock everything)
2. **Slice 2** & **Slice 3** (in parallel, or 2 first if USB is priority)
3. **Slice 4** (follows once Slice 3 or both 2 & 3 are done)
4. **Slice 5** (final; validates all slices)

---

## Next Steps

1. **User approval**: Review slices; adjust scope or dependencies if needed
2. **Create GitHub issues** for each slice (link to specs in docs/)
3. **Write full specs** (using SPEC_TEMPLATE.md) for Slice 1–5; user reviews & approves
4. **Assign & implement**: One slice per coding session; PR review before next slice

---

## Open Architecture Questions (For User)

- **USB HID stack**: Use TinyUSB library, or custom minimal impl?
- **SD card library**: Use existing Pico SD driver, or implement TASD writer directly?
- **Recording trigger**: GPIO pin (e.g., button on dev board), SPI command, or both?
- **Multi-recording**: Support simultaneous recordings to different files, or single-at-a-time?
- **Clock source**: Microsecond timestamps from hardware timer (Pico timer) or TASD-provided (host)?
- **Error recovery**: Watchdog reboot on SD failure, or graceful fallback to USB-only relay?

