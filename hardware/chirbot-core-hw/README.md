# chirbot-core-hw

> **Status:** planned — design not finalized. Cost target: < $100.

Hardware design for the CHIRBot core board. Expected major elements:

- RP2040 or RP2350 (SPI main)
- USB port to PC (power, configuration, input streaming)
- microSD slot
- Simple display + rudimentary buttons for on-device configuration
- Input module USB-C port and output module USB-C port (Debug Accessory Mode
  carrying SPI + module power)
- Optional visualization display output

See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md) and
[docs/specs/module-interface.md](../../docs/specs/module-interface.md).
