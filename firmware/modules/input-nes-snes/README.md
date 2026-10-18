# input-nes-snes

> **Status:** NES-to-core UART prototype implemented.

This is the existing input-only prototype image. The target architecture
consolidates NES/SNES input and output into one protocol-family firmware with
a physical role selector sampled at reset; that combined image is not
implemented yet. See the
[dual-role module decision](../../../docs/design/0004-dual-role-module-architecture.md).

NES/SNES controller input module. Polls native NES (8-bit) / SNES (16-bit)
controllers over their 5 V latched serial protocol and forwards input as
TASD-packetized SPI data to the core. Requires level shifting between the 5 V
controller bus and the 3.3 V RP2350B.

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).

## Prototype implementation

The current image reads a physical NES controller and emits changed states
as TASD `INPUT_MOMENT` reports on port 0, serving the latest frame as an SPI0
subnode. It emits the initial state even when no buttons are held.
SNES polling is not implemented yet.
The core's Controller Input mode forwards these live reports unchanged; the
output module accepts live port 0 separately from playback movie port 1.

Acquired states use the internal active-high serial-order button mask. The shared
codec serializes standard TASD active-low, MSB-first bytes (`FF` for released).
Update this image together with core/output when switching from the earlier
prototype's nonstandard TASD byte layout.

The controller signals use RJ45/8P8C contacts 8 (CLOCK, GP0), 6 (LATCH,
GP2), and 4 (DATA0, GP4). Contact 5 (DATA1) is not connected to a GPIO.

For the BSS138 bench interface, use module 3.3 V on LV, controller 5 V on
HV, and common ground. Never connect 5 V directly to an RP2350 GPIO.
CLOCK idles high and LATCH low. Each read uses a 12 us LATCH pulse, samples
A first, then generates eight CLOCK pulses with 6 us low/high periods.
Internal GPIO pulls are disabled; the translator supplies rail pull-ups.
Reads occur between core SPI transactions. Without core polling, the blocking
SPI service can pause acquisition; this is latest-state relay, not recording
every controller transition.

USB CDC prints `[in] ... buttons=0xNN` on initial/changed states (internal mask:
A=01, B=02, Select=04, Start=08, Up=10, Down=20, Left=40, Right=80).
An unread USB terminal does not block controller sampling. Select Controller
Input on the core to poll the module and forward its state to the output.

See the [NES-to-UART scenario](../../../docs/specs/nes-to-uart-scenario.md) for
level-shifter requirements, wiring, and expected core UART output.
