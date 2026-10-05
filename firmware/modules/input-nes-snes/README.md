# input-nes-snes

> **Status:** NES-to-core UART prototype implemented.

NES/SNES controller input module. Polls native NES (8-bit) / SNES (16-bit)
controllers over their 5 V latched serial protocol and forwards input as
TASD-packetized SPI data to the core. Requires level shifting between the 5 V
controller bus and the 3.3 V RP2350B.

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).

## Prototype implementation

The current image polls an NES controller on GP0/GP2/GP4, emits a TASD
`INPUT_MOMENT` whenever the button state changes, and serves the latest frame
as an SPI0 subnode. SNES polling is not implemented yet.

The controller signals use RJ45/8P8C contacts 8 (CLOCK, GP0), 6 (LATCH,
GP2), and 4 (DATA0, GP4). Contact 5 (DATA1) is not connected to a GPIO.

See the [NES-to-UART scenario](../../../docs/specs/nes-to-uart-scenario.md) for
level-shifter requirements, wiring, and expected core UART output.
