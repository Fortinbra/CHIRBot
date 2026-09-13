# CHIRBot module carrier

> **Status:** Planned - schematic, layout, connectors, and power budget are not
> finalized.

The module carrier is a passive backplane between the Core controller board
and the input/output modules. It carries buses, per-slot control signals,
power, and ground. It contains no routing MCU, does not interpret TASD, and
does not provide peer-to-peer module communication.

The accepted topology is defined by
[design decision 0003](../../docs/design/0003-carrier-and-multislot-bus-topology.md).
The electrical and mechanical slot contract belongs in the
[module-interface specification](../../docs/specs/module-interface.md).

## Slot banks

| Bank | Slots | Shared signals | Per-slot signals |
| --- | ---: | --- | --- |
| Input | 4 | SPI SCLK/MOSI/MISO, I2C SDA/SCL | CS, data-ready/IRQ, power, ground |
| Output | 4 | SPI SCLK/MOSI/MISO, I2C SDA/SCL | CS, reserved alert, power, ground |

The input and output SPI banks are electrically independent. The Core is the
sole SPI controller and asserts at most one CS in a bank at a time. Every
unselected module must keep MISO high-impedance. Each input IRQ is routed
independently to the Core.

The input and output I2C pairs are separate 3.3 V segments reserved for future
revisions. The carrier provides one pull-up location per segment; modules must
not fit fixed pull-ups by default. I2C remains disabled until addressing,
isolation, and recovery are specified.

## Carrier responsibilities

- Board-to-board connectors for the Core and all eight module slots
- Shared-bus fan-out with short stubs, controlled return paths, and optional
  source-termination footprints
- Inactive-state pulls on all module chip selects
- Per-slot module power distribution, protection, and any future switching or
  presence detection
- Mechanical keying or labeling that distinguishes input and output banks
- Test access for shared buses, every CS, every input IRQ, power rails, and
  ground

## Open design work

- Select connector families and assign contacts
- Define board outline, slot pitch, insertion direction, and retention
- Complete the eight-module steady-state and inrush power budget
- Decide per-slot MISO and future-I2C fault isolation
- Validate SPI loading, clock rate, and signal integrity with all slots fitted
- Define power-off-only or hot-swap behavior
