# chirbot-core-hw

> **Status:** planned — design not finalized. Cost target: < $100.

Hardware design for the CHIRBot core board. Expected major elements:

- RP2350B (sole SPI controller/main) + external QSPI flash
- USB port to PC (power, configuration, input streaming)
- microSD slot
- Simple display + rudimentary buttons for on-device configuration
- A board-to-board interface to the passive
  [module carrier](../chirbot-carrier-hw/) carrying two shared SPI banks,
  eight slot chip selects, four input data-ready/IRQ signals, two reserved I2C
  segments, module power control, and ground. Core firmware queries each
  populated slot before enabling its data path (connector selection pending -
  see [design decision 0003](../../docs/design/0003-carrier-and-multislot-bus-topology.md))
- Optional visualization display output

See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md) and
[docs/specs/module-interface.md](../../docs/specs/module-interface.md).
