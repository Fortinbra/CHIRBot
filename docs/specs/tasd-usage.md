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

The input moment uses millisecond indexing from input-module boot. Its input is
one active-high byte in NES serial order:

| Bit | Button |
| ---: | --- |
| 0 | A |
| 1 | B |
| 2 | Select |
| 3 | Start |
| 4 | Up |
| 5 | Down |
| 6 | Left |
| 7 | Right |

An event is emitted on startup and whenever this byte changes. The live SPI
envelope is defined separately in [SPI matrix protocol](spi-matrix-protocol.md).

### SNES output mapping

The output module also accepts `SNES_STANDARD` (`0x0201`) with two active-high
input bytes. Bits are shifted least-significant first in this provisional order:

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

The current input module emits NES data only. The SNES mapping is implemented
for core playback and future SNES input support.
