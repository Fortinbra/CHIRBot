# hardware/

Hardware designs (KiCad projects, BOMs, fabrication outputs) for CHIRBot
components.

> **Status:** hardware designs are not finalized. Every board is based on the
> RP2350B (standard MCU — see
> [docs/design/0002](../docs/design/0002-mcu-selection-rp2350b.md)).

| Directory | Purpose |
|---|---|
| [chirbot-core-hw/](chirbot-core-hw/) | CHIRBot core board |
| [modules/](modules/) | Input/output module boards (mirrors `firmware/modules/` naming) |

Components start as folders here and are promoted to standalone `<component>-hw`
repos once they have real content. See
[docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md) §3.
