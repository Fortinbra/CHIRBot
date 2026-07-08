# firmware/modules/

Input/output module firmware. Each module is an SPI subnode (RP2040/RP2350)
that converts between a native device protocol and TASD-packetized SPI data,
docked board-to-board onto the core (no link cables).

| Directory | Direction | Native protocol |
|---|---|---|
| [input-usb/](input-usb/) | Input | USB host (PIO) — modern/adaptive controllers |
| [input-nes-snes/](input-nes-snes/) | Input | NES/SNES 5 V serial |
| [input-gc-n64/](input-gc-n64/) | Input | GameCube/N64 3-wire |
| [output-usb/](output-usb/) | Output | USB device — HID modes, controller spoofing |
| [output-nes-snes/](output-nes-snes/) | Output | NES/SNES serial (controller/scope/mouse) |
| [output-gc-n64/](output-gc-n64/) | Output | GameCube/N64 3-wire |

Additional planned module types (Bluetooth/wireless, Genesis/C64/Atari, PS/2/AT
keyboard, telegraph) get folders when work begins. Naming convention:
`<direction>-<device>` here, `chirbot-module-<direction>-<device>` when promoted
to a standalone repo. See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md) §3.3.
