# chirbot-core

> **Status:** planned — no code yet. Hardware design not finalized.

Firmware for the CHIRBot core board (RP2040/RP2350). Responsibilities:

- **SPI matrix switch (main)** — routes TASD-packetized input between sources
  (input module, PC stream, microSD playback) and sinks (output module,
  visualization, microSD recording) with a ~1 ms relay target
- **USB to PC** — device power, matrix configuration, virtual input streaming,
  microSD exposure as mass storage
- **microSD storage** — TASD macro/run recording and playback
- **Config display + simple controls** — on-device recording/playback/settings
- **Module management** — USB-C DAM attach/detach, power, handshake, config

Depends on [chirbot-common](../chirbot-common/) and the
[TASD library](../TASD/). See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).
