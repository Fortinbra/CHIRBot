# input-gc-n64

> **Status:** planned — no code yet.

GameCube/N64 controller input module. Polls native controllers over the
single-wire (3-wire connector) Joybus protocol — a natural fit for PIO — and
forwards input as TASD-packetized SPI data to the core.

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).
