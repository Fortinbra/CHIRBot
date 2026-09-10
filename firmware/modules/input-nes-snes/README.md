# input-nes-snes

> **Status:** planning complete, project skeleton created.

NES/SNES controller input module. Polls native NES (8-bit) / SNES (16-bit)
controllers over their 5 V latched serial protocol and forwards input as
TASD-packetized SPI data to the core. Requires level shifting between the 5 V
controller bus and the 3.3 V RP2350B.

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).

## Implementation

Project skeleton created with Pico SDK structure. See `PLAN.md` for details.
