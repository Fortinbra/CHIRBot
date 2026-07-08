# CHIRBot
Computer-Human Input Relay Bot — an open source input relay device that remaps
any input device to any output device, with input recording, playback, and
realtime visualization.

- Project site: <https://chirbot.com/>
- Architecture & repository structure: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

This is the parent repo for the CHIRBot project. Components live here as
folders and are promoted to standalone repos (linked as submodules) as they
mature. All firmware is built on the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
targeting the RP2040/RP2350.

| Directory | Contents |
|---|---|
| [docs/](docs/) | Architecture, specs, design decisions |
| [firmware/](firmware/) | Core, shared libs, module firmware; [TASD](https://github.com/Fortinbra/TASD) submodule |
| [hardware/](hardware/) | Core and module board designs |
| [host/](host/) | PC app and CLI tooling |
| [tools/](tools/) | Project-wide dev tooling |
