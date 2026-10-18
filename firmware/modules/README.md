# firmware/modules/

Each protocol-specific module is one RP2350B SPI subnode with a single firmware
image that supports input and output roles. A physical switch is sampled at
startup/reset; exactly one role is active until the next reset. In input role,
the module converts its native device protocol into TASD and asserts
data-ready while unread data is pending. In output role, it receives TASD and
responds to native poll edges locally. The core owns all routing, recording,
playback, and policy and initiates every SPI transfer.

Modules return a self-description record that reports the selected role.
Every module must keep MISO high-impedance whenever its chip select is
inactive. The carrier slot organization is unresolved; see
[design decision 0004](../../docs/design/0004-dual-role-module-architecture.md).

| Protocol-family firmware | Input role | Output role | Current source status |
| --- | --- | --- | --- |
| NES/SNES | NES controller sampling; SNES planned | NES/SNES controller emulation | Separate prototype images remain in [input-nes-snes/](input-nes-snes/) and [output-nes-snes/](output-nes-snes/); consolidation is future work |
| USB | USB host for modern/adaptive controllers | USB device HID modes and future controller presentation | Planned |
| GameCube/N64 | Acquire Joybus-family input | Present Joybus-family output | Planned |

Additional protocol families (Bluetooth/wireless, Genesis/C64/Atari,
PS/2/AT keyboard, telegraph) get one module product per family when work
begins. See [architecture](../../docs/ARCHITECTURE.md) section 3.3 for the
module naming convention.
