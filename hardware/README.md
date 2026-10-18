# hardware/

Hardware designs (KiCad projects, BOMs, fabrication outputs) for CHIRBot
components.

> **Status:** hardware designs are not finalized. Every active controller and
> module board is based on the RP2350B (standard MCU - see
> [docs/design/0002](../docs/design/0002-mcu-selection-rp2350b.md)).

| Directory | Purpose |
| --- | --- |
| [chirbot-core-hw/](chirbot-core-hw/) | CHIRBot core board |
| [chirbot-carrier-hw/](chirbot-carrier-hw/) | Passive module carrier; role-specific banks versus universal slots remain undecided |
| [modules/](modules/) | Protocol-family dual-role module boards (mirrors `firmware/modules/` naming) |

Planning documents:

- [Three-MCU architecture prototype](PROTOTYPE.md) — sourced bench prototype
  using PGA2350 boards, microSD, and SNES level translation

Components start as folders here and are promoted to standalone `<component>-hw`
repos once they have real content. See
[docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md) §3.
