# chirbot-core-hw

> **Status:** planned — design not finalized. Cost target: < $100.

Hardware design for the CHIRBot core board. Expected major elements:

- RP2350B (sole SPI controller/main) + external QSPI flash
- USB port to PC (power, configuration, input streaming)
- microSD slot
- Simple display + rudimentary buttons for on-device configuration
- A board-to-board interface to the passive
  [module carrier](../chirbot-carrier-hw/). Its final bus banks, slot count,
  chip-select and role-dependent data-ready/IRQ routing, reserved I2C, module
  power control, and ground depend on the unresolved carrier topology. Core
  firmware queries each populated slot and validates its switch-selected role
  before enabling its data path (see
  [design decisions 0003](../../docs/design/0003-carrier-and-multislot-bus-topology.md)
  and
  [0004](../../docs/design/0004-dual-role-module-architecture.md)).
- Optional visualization display output

See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md) and
[docs/specs/module-interface.md](../../docs/specs/module-interface.md).
