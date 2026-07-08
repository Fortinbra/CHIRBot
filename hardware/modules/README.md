# hardware/modules/

Hardware designs for input/output module boards.

> **Status:** no designs yet. Cost target: < $40 per module.

Folder naming mirrors [firmware/modules/](../../firmware/modules/)
(e.g., `input-nes-snes/`). Each module provides the device-specific physical
and electrical adaptation: native connector, level shifting/voltage handling,
and an RP2040/RP2350 SPI subnode attached to the core via USB-C Debug Accessory
Mode. Output modules are designed with controller cable extenders in mind.

See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).
