# firmware/modules/

Input/output module firmware. Each module is an SPI subnode (RP2350B)
that converts between a native device protocol and TASD-packetized SPI data,
connected board-to-board to one slot on the passive module carrier (no link
cables). Up to four input and four output modules share direction-specific SPI
banks, with one core-controlled chip select per slot.

Each module has one data-path responsibility: an input module translates one
controller protocol into TASD, while an output module translates TASD into one
console-facing protocol. Mapping, routing, recording, playback, and policy stay
in the core. All modules return a self-description record when the core queries
them. Input modules also assert a dedicated data-ready signal while unread TASD
data is pending; the core remains the sole initiator of all SPI transfers. A
module must keep MISO high-impedance whenever its CS is inactive. Reserved I2C
pins are not used by the current firmware contract.

| Directory | Direction | Native protocol |
| --- | --- | --- |
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
