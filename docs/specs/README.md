# docs/specs/

Project-wide technical specifications shared by firmware, hardware, and host
software. Planned documents:

| Spec | Contents | Status |
| --- | --- | --- |
| [SPI matrix protocol](spi-matrix-protocol.md) | Framing, timing budget, main/subnode roles, discovery | prototype |
| [Module interface](module-interface.md) | Docking pinout, mechanicals, power, hot-swap policy, handshake/versioning | outline |
| [TASD usage](tasd-usage.md) | How CHIRBot uses or extends the [TASD format](https://tasd.io/) | prototype |
| [Clock domains](clock-domains.md) | Poll-edge timing model and buffering behavior | outline |
| [NES-to-UART scenario](nes-to-uart-scenario.md) | First end-to-end wiring and acceptance test | prototype |

An **outline** names the contract and unresolved decisions but is not stable
enough to implement against. Each document must move through draft review
before it can be marked accepted.
