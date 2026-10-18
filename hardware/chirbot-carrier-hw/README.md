# CHIRBot module carrier

> **Status:** Planned - schematic, layout, connectors, and power budget are not
> finalized.

The module carrier is a passive backplane between the Core controller board
and dual-role protocol modules. It carries buses, per-slot control signals,
power, and ground. It contains no routing MCU, does not interpret TASD, and
does not provide peer-to-peer module communication.

The earlier four-input/four-output topology is recorded in
[design decision 0003](../../docs/design/0003-carrier-and-multislot-bus-topology.md).
Following the dual-role module decision in
[design decision 0004](../../docs/design/0004-dual-role-module-architecture.md),
the production slot organization is open. The electrical and mechanical slot
contract belongs in the
[module-interface specification](../../docs/specs/module-interface.md).

## Candidate slot organizations

| Bank | Slots | Shared signals | Per-slot signals |
| --- | ---: | --- | --- |
| Input | 4 | SPI SCLK/MOSI/MISO, I2C SDA/SCL | CS, data-ready/IRQ, power, ground |
| Output | 4 | SPI SCLK/MOSI/MISO, I2C SDA/SCL | CS, reserved alert, power, ground |

The table records the earlier role-specific-bank candidate, not a finalized
production design. A module's physical switch selects input or output at
reset. Under this candidate, it must be docked in the matching bank. A
universal-slot alternative would allow either selected role in each slot but
requires a new bus/IRQ routing design. Slot count and mismatch behavior are
open.

Regardless of organization, the Core is the sole SPI controller, asserts at
most one CS on any shared bus, and every unselected module keeps MISO
high-impedance. Data-ready is used only in input role.

If separate role-specific banks are retained, their I2C pairs are separate
3.3 V segments reserved for future revisions. A universal-slot carrier would
need its own segment and pull-up policy. I2C remains disabled until addressing,
isolation, and recovery are specified; modules must not fit fixed pull-ups by
default.

## Carrier responsibilities

- Board-to-board connectors for the Core and the selected universal or
  role-specific slot count
- Shared-bus fan-out with short stubs, controlled return paths, and optional
  source-termination footprints
- Inactive-state pulls on all module chip selects
- Per-slot module power distribution, protection, and any future switching or
  presence detection
- Mechanical keying or labeling that prevents role/slot confusion if separate
  banks are retained
- Test access for shared buses, every CS, role-dependent IRQ contacts, power
  rails, and ground

## Open design work

- Select connector families and assign contacts
- Define board outline, slot pitch, insertion direction, and retention
- Complete the selected carrier's steady-state and inrush power budget
- Decide per-slot MISO and future-I2C fault isolation
- Validate SPI loading, clock rate, and signal integrity with all slots fitted
- Define power-off-only or hot-swap behavior
