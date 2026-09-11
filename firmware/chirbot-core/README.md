# chirbot-core

> **Status:** First SPI-to-UART relay prototype implemented. Hardware design is
> not finalized.

Firmware for the CHIRBot core board (RP2350B). Responsibilities:

- **SPI matrix switch (main)** — routes TASD-packetized input between sources
  (input module, PC stream, microSD playback) and sinks (output module,
  visualization, microSD recording) with a ~1 ms relay target
- **USB to PC** — device power, matrix configuration, virtual input streaming,
  microSD exposure as mass storage
- **microSD storage** — TASD macro/run recording and playback
- **Config display + simple controls** — on-device recording/playback/settings
- **Module management** — docked module detection, power, handshake, config

Depends on [chirbot-common](../chirbot-common/) and the
[TASD library](../TASD/). See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).

The current image polls an input module over SPI0, validates and decodes its
TASD frame, prints every packet over UART0, and forwards new frames over SPI1.
See the [NES-to-UART scenario](../../docs/specs/nes-to-uart-scenario.md) for
wiring, UF2 paths, and expected output.
