# docs/specs/

Project-wide technical specifications shared by firmware, hardware, and host
software. Planned documents:

| Spec | Contents | Status |
| --- | --- | --- |
| [SPI matrix protocol](spi-matrix-protocol.md) | Framing, shared-bank scheduling, controller/peripheral roles, discovery, descriptor encoding, data-ready service | prototype |
| [Module interface](module-interface.md) | Carrier and slot pinouts, shared SPI/I2C electrical rules, mechanicals, power, descriptor fields and compatibility | outline |
| [TASD usage](tasd-usage.md) | How CHIRBot uses or extends the [TASD format](https://tasd.io/) | prototype |
| [Clock domains](clock-domains.md) | Poll-edge timing model and buffering behavior | outline |
| [NES-to-UART scenario](nes-to-uart-scenario.md) | First end-to-end wiring and acceptance test | prototype |
| [SD card storage](sd-storage.md) | microSD hardware, filesystem, and TASD recording/playback on the core | draft |
| [Startup menu and mode selection](menu-system.md) | Three-button menu picking Controller Input, File Playback, or SD Tools at boot, with pin/wiring assignment | draft |

An **outline** names the contract and unresolved decisions but is not stable
enough to implement against. Each document must move through draft review
before it can be marked accepted.
