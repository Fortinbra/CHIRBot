# Specification Template

**Issue:** #{issue-number}  
**Title:** {Brief title describing the feature}  
**Status:** Draft / Review / Approved / Implemented

---

## Problem Statement

Clearly describe the problem this feature solves or the gap it fills in the system.

Example:
> "The core relay currently has no way to persistently store input events. We need a mechanism to record TASD-formatted input packets to microSD card so that recorded sessions can be replayed for testing and debugging."

---

## Design Goals

List the key objectives for this feature. Focus on *what* we're trying to achieve, not *how*.

Example:
- Record input packets with microsecond-level timestamps
- Support stop/resume without data loss
- Enable offline replay of captured sessions
- Keep core relay logic decoupled from storage implementation

---

## API / Protocol

Define the public interfaces, functions, message formats, or wire protocols introduced by this feature.

### Input / Output Contracts
Specify what data flows in and what data flows out. Use actual struct definitions or protocol examples.

Example (TASD Input via SPI):
```
SPI Frame (host → core):
  [0]    = Frame type (0x01 = input event)
  [1:5]  = Timestamp (uint32, microseconds)
  [6:7]  = Button state (uint16, bitmask)
  [8:9]  = Analog X (int16, -32768 to +32767)
  [10:11] = Analog Y (int16, -32768 to +32767)
```

### Function Signatures / Module Boundaries
If this feature defines a new module or subsystem, list key entry points.

Example:
```c
// microSD module interface
typedef struct {
    uint32_t timestamp_us;
    uint16_t buttons;
    int16_t analog_x;
    int16_t analog_y;
} InputPacket;

bool sd_write_packet(InputPacket *pkt);
bool sd_start_recording(const char *filename);
bool sd_stop_recording(void);
```

---

## Data Structures

Define any new types, enums, or persistent data formats.

Example:
```c
// Recording state machine
typedef enum {
    REC_IDLE,
    REC_ACTIVE,
    REC_ERROR
} RecordingState;

// On-card file format (microSD)
// Header: "TASD" + version + start timestamp
// Body: stream of InputPacket structs
// Each packet is 12 bytes (timestamp_us + buttons + analog_x/y)
```

---

## Validation & Acceptance Criteria

Define *exactly* how to test this feature. Each criterion must be testable in isolation.

Example:
- [ ] Core can receive a TASD input packet via SPI (bytes in expected positions, no corruption)
- [ ] Core correctly parses timestamp, buttons, and analog values from a packet
- [ ] Core relays parsed packet to output device (USB HID or next module) within 1ms
- [ ] Core handles malformed packets gracefully (logs, does not crash)
- [ ] Recorded packets on microSD can be read back and bit-verified against originals

---

## Open Questions

List unresolved design decisions or ambiguities that need clarification before implementation.

Example:
- Should recording start/stop be triggered via SPI command or via a GPIO pin?
- How large should the microSD buffer be before flushing to disk?
- Should we support multiple simultaneous recordings, or one at a time?

---

## Implementation Notes (Optional)

Add any architectural constraints, clock domain details, or internal trade-offs here.

Example:
> "Output device drives the poll edge at ~125 Hz. Core must relay at <1ms latency to avoid button double-bouncing. Use SPI DMA for throughput, but ensure synchronous ACK on packet receipt."

---

## References

- [TASD Format](https://tasd.io) — Input/output protocol specification
- [CHIRBot Architecture](../README.md) — System overview
- [Previous Related Issues](#{link}) — Context from earlier work
