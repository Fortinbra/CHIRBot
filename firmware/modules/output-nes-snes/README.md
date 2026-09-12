# output-nes-snes

> **Status:** NES/SNES output prototype implemented.

NES/SNES output module. Receives TASD-packetized SPI data from the core and
responds to the console's latch/clock as a native controller — 8-bit (NES) or
16-bit (SNES) shifting, adjustable when acting as a Super Scope or mouse. The
console's latch is the poll edge and owns the clock domain
(see [docs/specs/clock-domains.md](../../../docs/specs/clock-domains.md)).

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).

## Prototype implementation

The module receives versioned, CRC-protected TASD frames from the core as an
SPI0 subnode. It validates `PORT_CONTROLLER` and `INPUT_MOMENT`, retains the
latest complete state, and uses PIO0 to answer console latch/clock polls without
waiting for another core transaction.

Supported profiles:

- Standard NES: one input byte, eight serial bits
- Standard SNES: two input bytes, sixteen serial bits

Core-facing pins are GP16 MOSI input, GP17 CS, GP18 SCLK, and GP19 MISO output.
Console-facing pins are GP2 LATCH input, GP3 CLOCK input, and GP4 DATA output.
See the [NES-to-UART scenario](../../../docs/specs/nes-to-uart-scenario.md) for
the complete three-board wiring and level-shifter requirements.

Accessory protocols such as Four Score, multitap, mouse, and Super Scope are
not supported.
