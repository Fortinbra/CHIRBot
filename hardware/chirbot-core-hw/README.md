# chirbot-core-hw

> **Status:** planned — design not finalized. Cost target: < $100.

Hardware design for the CHIRBot core board. Expected major elements:

- RP2040 or RP2350 (SPI main)
- USB port to PC (power, configuration, input streaming)
- microSD slot
- Simple display + rudimentary buttons for on-device configuration
- Input module and output module board-to-board docking connectors carrying
  SPI + module power (family selection pending — see
  [docs/design/0001](../../docs/design/0001-module-link-bus-and-connector.md))
- Optional visualization display output

See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md) and
[docs/specs/module-interface.md](../../docs/specs/module-interface.md).
