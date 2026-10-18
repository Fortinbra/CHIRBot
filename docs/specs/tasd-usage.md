# TASD usage

> **Status:** Prototype NES/SNES live mapping implemented.

This specification will define how CHIRBot uses the
[TASD format](https://tasd.io/) for recordings and how TASD concepts relate to
the live module link. Serialization is provided by the external
[Fortinbra/TASD](https://github.com/Fortinbra/TASD) library.

## Required decisions

- Required and optional metadata for recordings created by CHIRBot
- Input sample representation, controller/port identity, and timebase
- Recording start/stop, incomplete-file recovery, and storage layout
- Playback validation, unsupported packets, and forward compatibility
- Boundary between TASD file packets and the live SPI transport envelope
- Representation of rumble, motion, authentication, and other return data
- Upstream extension process and handling of experimental packet keys
- Golden files and conformance tests against the upstream specification

CHIRBot-specific extensions should be proposed upstream and must not silently
reuse keys assigned by the TASD specification.

## Prototype live mapping

Each SPI type-1 payload is a complete, independently parseable TASD document:

1. Standard 7-byte TASD header.
2. `PORT_CONTROLLER` for port 0 with controller type `NES_STANDARD` (`0x0101`).
3. One `INPUT_MOMENT` for port 0.

The input moment uses millisecond indexing from input-module boot. TASD stores
one active-low byte, with A in bit 7 through Right in bit 0:

| Bit | Button |
| ---: | --- |
| 7 | A |
| 6 | B |
| 5 | Select |
| 4 | Start |
| 3 | Up |
| 2 | Down |
| 1 | Left |
| 0 | Right |

`FF` means released; `EF` means START only. The shared codec converts at
serialization boundaries to/from the internal active-high serial-order layout
(A in bit 0). Display and output helpers consume the internal layout, never raw
TASD bytes. Live input and file playback use the same on-wire TASD format.

An event is emitted on startup and whenever this byte changes. The live SPI
envelope is defined separately in [SPI matrix protocol](spi-matrix-protocol.md).

### SNES output mapping

The output module also accepts `SNES_STANDARD` (`0x0201`). Both TASD input bytes
are active-low, MSB-first: byte 0 contains B, Y, Select, Start, Up, Down, Left,
Right; byte 1 contains A, X, L, R in bits 7..4, with bits 3..0 held at 1.
The codec reverses/inverts each byte into the internal serial-order layout below.
The serializer shifts these internal bits least-significant first:

| Bit | Button |
| ---: | --- |
| 0 | B |
| 1 | Y |
| 2 | Select |
| 3 | Start |
| 4 | Up |
| 5 | Down |
| 6 | Left |
| 7 | Right |
| 8 | A |
| 9 | X |
| 10 | L |
| 11 | R |
| 12-15 | Inactive/reserved |

The current NES input prototype emits NES data only. SNES input support is
planned for the consolidated NES/SNES module family.

The shared `chirbot_controller_tasd` codec owns this live controller-state
contract. Both selected roles of a protocol-specific module use the same TASD
representation; its host pipeline test verifies NES and SNES documents
through CHIRBot link framing.

## Console-clocked file playback

File playback uses the acknowledged playback commands in
[SPI matrix protocol](spi-matrix-protocol.md), keeping up to four reports queued
at the output. The console's LATCH, not a core timer or assumed frame rate,
advances the recording. See [clock domains](clock-domains.md).

`INPUT_CHUNK` is split into complete controller reports: one byte per NES poll,
two bytes per SNES poll. A chunk with a partial report is rejected.
Playback selects player 1 (`kPlaybackControllerPort = 1`), matching the output
module's configured port. Other ports' metadata and reports are skipped; an
interleaved two-player recording does not consume two polls per movie frame.
Frame-indexed `INPUT_MOMENT` packets must have strictly increasing indices on
the selected port. The first index is the recording origin; gaps after it queue
repeats of the preceding state. Millisecond and other timebases are rejected,
as are profile changes on the selected port during playback. This is a single-port prototype,
not a multi-controller or subframe movie scheduler.

The 448 KiB file buffer bounds read-ahead; larger files are rejected explicitly,
never played truncated. Reports not yet sent remain
in that buffer while the output queue is full. End-of-file waits for the queue
to drain. On abort or playback error, the core requests an acknowledged stop
that flushes the queue and releases buttons. Per-report output UART logging is
disabled for playback to avoid delaying queue replenishment.

NES/SNES `LATCH_FILTER` packets before input data are validated as two-byte
big-endian microsecond intervals and sent to the output before playback begin.
Repeated console polls inside the interval repeat the same input rather than
consuming another report. Mid-playback filter changes are rejected.
`3922M.tasd` requests 8000 us; ignoring this metadata can consume the recording
many times too quickly when the game polls repeatedly.

NES/SNES `CLOCK_FILTER` packets are length-checked and produce an explicit
unsupported-filter warning (units: 100 ns). Clock filtering is not yet
implemented. Matching the published movie duration additionally requires
correct console/game/reset alignment; report counts alone do not prove sync.
