# firmware/

Firmware for all CHIRBot components, built on the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
targeting the RP2040/RP2350.

| Directory | Purpose |
|---|---|
| [TASD/](TASD/) | TASD file/packet serialization library (git submodule → [Fortinbra/TASD](https://github.com/Fortinbra/TASD)) |
| [chirbot-common/](chirbot-common/) | Shared libraries: SPI matrix link protocol, module handshake/config, TASD wrappers |
| [chirbot-core/](chirbot-core/) | CHIRBot core firmware (SPI main, USB to PC, microSD, display/controls) |
| [modules/](modules/) | Input/output module firmware (SPI subnodes) |

Components start as folders here and are promoted to standalone repos
(referenced as submodules) once they have real content. See
[docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md) §3.
