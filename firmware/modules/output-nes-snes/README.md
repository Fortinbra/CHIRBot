# output-nes-snes

> **Status:** planned — no code yet.

NES/SNES output module. Receives TASD-packetized SPI data from the core and
responds to the console's latch/clock as a native controller — 8-bit (NES) or
16-bit (SNES) shifting, adjustable when acting as a Super Scope or mouse. The
console's latch is the poll edge and owns the clock domain
(see [docs/specs/clock-domains.md](../../../docs/specs/clock-domains.md)).

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).
