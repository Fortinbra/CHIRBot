# CHIRBot
Computer-Human Input Relay Bot — an open source input relay device that remaps
any input device to any output device, with input recording, playback, and
realtime visualization.

- Project site: <https://chirbot.com/>
- Documentation index: [docs/README.md](docs/README.md)
- Architecture & repository structure: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- Project roadmap: [docs/ROADMAP.md](docs/ROADMAP.md)

> **Status:** Early design and prototyping. The architecture is still evolving;
> accepted decisions are recorded in [docs/design/](docs/design/). The website
> describes the original concept, while this repository is the source of truth
> for current technical decisions.

This is the parent repo for the CHIRBot project. Components live here as
folders and are promoted to standalone repos (linked as submodules) as they
mature. All firmware is built on the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk);
all boards use the RP2350B as the standard MCU.

| Directory | Contents |
|---|---|
| [docs/](docs/) | Architecture, specs, design decisions |
| [firmware/](firmware/) | Core, shared libs, module firmware; [TASD](https://github.com/Fortinbra/TASD) submodule |
| [hardware/](hardware/) | Core and module board designs |
| [host/](host/) | PC app and CLI tooling |
| [tools/](tools/) | Project-wide dev tooling |

## Getting the repository

Clone with submodules so the TASD library is available:

```sh
git clone --recurse-submodules https://github.com/Fortinbra/CHIRBot.git
```

For an existing clone:

```sh
git submodule update --init --recursive
```
