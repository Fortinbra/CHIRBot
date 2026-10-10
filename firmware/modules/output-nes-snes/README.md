# output-nes-snes

> **Status:** NES/SNES output prototype implemented.

NES/SNES output module. Receives TASD-packetized SPI data from the core and
responds to the console's latch/clock as a native controller — 8-bit (NES) or
16-bit (SNES) shifting. The
console's latch is the poll edge and owns the clock domain
(see [docs/specs/clock-domains.md](../../../docs/specs/clock-domains.md)).

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).

## Prototype implementation

The module receives versioned, CRC-protected TASD frames from the core as an
SPI0 subnode. It validates `PORT_CONTROLLER` and `INPUT_MOMENT`, retains the
latest complete state, and uses PIO0 to answer console latch/clock polls without
waiting for another core transaction.
File playback instead uses an acknowledged four-report FIFO consumed by console
polls; live and playback commands are distinct.

Supported profiles:

- Standard NES: one input byte, eight serial bits
- Standard SNES: two input bytes, sixteen serial bits

Core-facing pins are GP16 MOSI input, GP17 CS, GP18 SCLK, and GP19 MISO output.
Console-facing pins are GP2 LATCH input, GP0 CLOCK input, and GP4 DATA output.
The current bench fixture has returned to the BSS138 translator, which has no
OE input. GP10 is unused, configured as an input with a pull-down, and must be
left disconnected. Firmware no longer drives the former TXS0108E OE high.
GP2 and GP0 have internal pulls disabled and rely on the BSS board's pull-ups
to their respective LV/HV rails. LV is module 3.3 V, HV is console 5 V, with
common ground; do not join console 5 V to the module's supply.
The BSS fixture remains electrically unqualified and has no firmware-controlled
isolation. Power down both devices before wiring changes and avoid connected
signals when either translator rail is unpowered.
The RJ45/8P8C contacts are 6 (LATCH), 8 (CLOCK), and 4 (DATA0); contact 5
(DATA1) is not connected to a GPIO. The PIO jump pin selects LATCH and the input
pin base selects CLOCK independently, so they need not be adjacent.
See the [NES-to-UART scenario](../../../docs/specs/nes-to-uart-scenario.md) for
the complete three-board wiring and level-shifter requirements.

Accessory protocols such as Four Score, multitap, mouse, and Super Scope are
not supported.

## Console behavior

The firmware starts with all buttons released and enables console output before
USB logging, without waiting for a terminal or core transaction. USB startup
messages may therefore be missed. USB writes have no backpressure wait, so an
unread terminal cannot hold up SPI reception; diagnostic text is best-effort
and may be dropped. Only validated standard NES/SNES states for the configured
TASD port can change the output profile: live type-1 reports use **port 0**
as emitted by the input module; playback begin/state use **port 1** for the
movie's player-1 reports. These source-specific selectors are defined in
`include/controller_output_state.hpp`. Both feed the same physical console
controller connector; movie port 2 and other unmatched reports remain ignored.

For NES, LATCH high continuously reloads the most recent state and presents A.
LATCH low freezes the report. DATA stays stable while CLOCK is low (the console
is reading it), then advances on the rising edge:
A, B, Select, Start, Up, Down, Left, Right. Pressed buttons drive DATA low.
After the eighth rising edge, DATA stays **low**, matching an original NES
controller's zero-filled shift register. The console inverts this to CPU bit
value 1. A fresh LATCH restarts a report even if the previous read stopped early.
See the [NESdev standard-controller reference](https://www.nesdev.org/wiki/Standard_controller).

SNES retains sixteen-bit shifting and **high** DATA after the report. It uses
GPIO output inversion with an active-high shift word; NES uses a masked
active-low byte without inversion. Both share one 19-instruction PIO program.
Core updates received during a read are queued for the next LATCH, not spliced
into the current report. With no new frames, polls repeat the last valid state.
These latest-state rules apply to live input. Changing between NES and SNES restarts the state machine; change the source
profile only when console polling is stopped.

### File playback

Playback uses a separate 28-instruction PIO program, loaded in place of the
live program, plus a four-instruction LATCH-filter timer on another PIO0 state
machine. It consumes one FIFO entry on an eligible LATCH and holds that
report throughout held LATCH and subsequent CLOCK pulses. It never clears the
FIFO on a new playback state. Up to four pending reports retain their order;
when full, new states are not acknowledged and the core waits/retries.
Without a new entry, a poll repeats the previous state.

The core configures the file's latch-filter interval before begin. For
`3922M.tasd`, the interval is 8000 microseconds: additional polls during that
interval restart the same report without advancing the queue. The timer
cannot advance playback itself. With a zero interval, every LATCH advances
as before. Neither filtering nor playback uses GP10 or changes BSS wiring.
Clock filtering is not yet implemented; the core warns when a file requests it.

The core reads acknowledgements and free-slot status over the existing MISO
wire. There is no new GPIO or console-clock generation. Both core and output
firmware must be flashed together. Frame-indexed moments and NES/SNES chunks
are supported; timestamps and selected-port profile changes within a movie are rejected.
Core playback selects port 1 and skips other players in interleaved movies.
End-of-file waits until the last queued report is latched. SELECT flushes pending
reports and releases buttons. See [clock domains](../../../docs/specs/clock-domains.md)
and [SPI matrix protocol](../../../docs/specs/spi-matrix-protocol.md).

Bench-check playback with the console stopped: the queue must fill without
advancing the recording. Pulse LATCH once for each of four different queued
states and verify ordered reports. Hold LATCH high and verify no additional
entry is consumed, then pause/resume polling and verify no queued state is
skipped. Check cancellation with a full queue and completion with the final
report still pending. Stop polling before switching profiles.

## Flashing and bench acceptance

1. Run the workspace **Compile Project** task and flash
   `build/firmware/modules/output-nes-snes/output_nes_snes.uf2` to the output
   PGA2350.
2. With power off, wire the dedicated core SPI link and console signals as
   described in the [NES-to-UART scenario](../../../docs/specs/nes-to-uart-scenario.md).
   GP2/GP0 must receive level-shifted LATCH/CLOCK; GP4 drives translated DATA0.
   RP2350 GPIO is **not 5 V tolerant**. Use the fixed-direction LVC/AHCT buffers
   in [the hardware prototype guide](../../../hardware/PROTOTYPE.md), keep DATA
   output-enable disabled during reset, and do not join console and system 5 V.
   For the current unqualified BSS bench exception, leave GP10 disconnected.
   A future fixed-direction buffer fixture needs its own reset-safe enable
   circuit and matching firmware; do not use this image to enable a TXS board.
3. Start with a standard NTSC NES and controller port 1. Verify the cable
   contact mapping by continuity, not cable color or an assumed connector view.
   PAL diode-protected ports and accessories are not qualified.
4. Capture LATCH, CLOCK, and DATA with a logic analyzer. With no core state,
   expect eight high DATA bits followed by low DATA. Press A only: the first
   bit must be low, the next seven high, and every extra bit low. Repeat for
   every button and a combination such as A + Start + Right.
5. Confirm DATA changes after CLOCK rises, not when it falls. Hold LATCH high:
   reads must keep returning A. Stop a read early and re-latch: it must restart
   at A. Update the input during a read: that report stays unchanged and the
   next report reflects the update.
6. Verify press/release behavior in a game, including with USB logging
   disconnected. Scope the translated signals for voltage, rise time, ringing,
   and reset/power-off behavior before treating the fixture as qualified.

The output holds its last state indefinitely if the core link stops; there is
no watchdog release in this prototype. Live relay remains unacknowledged;
file playback has acknowledgement and backpressure as described above.

## Serial diagnostics

Existing startup, button-state, and link/error logs go to both USB CDC and
UART1 at **115200 baud, 8 data bits, no parity, 1 stop bit**, without hardware
flow control. Connect a **3.3 V logic USB-to-UART adapter**:

| Output module | Adapter | Purpose |
| --- | --- | --- |
| GP6 | RX | Module TX / diagnostic output |
| GP5 | TX (optional) | Module RX; no terminal commands implemented |
| GND | GND | Common reference |

GP6 uses the RP2350 alternate UART1 TX function; GP5 uses UART1 RX.
GP4 remains console DATA0. Do not use RS-232 voltage levels or a 5 V TX signal,
and leave the adapter's power pin disconnected when the module is powered
separately. Open the terminal before resetting the module to see
`NES/SNES output module ready`.

Each accepted new live state prints an `[OUT seq=...]` button report. Playback
states are not printed individually. The `[out]`
summary reports transfers, CS timeouts, frame decode errors, and rejected
controller states. Rejection details are limited to one report per two seconds
and include the codec result and complete bounded TASD payload in hex.
Result -2 means unsupported controller, -3 malformed document, and -5 missing
controller/input packets. Result 0 means the document is
valid. Other ports are intentionally ignored and counted as `other_ports`,
not state errors, so player-2 packets cannot overwrite player-1 DATA.
Port/type/input fields are
only decoded when the result is 0; otherwise they are initialized zeros.
The summary runs after SPI transfers return, so an unwired
or inactive core link can leave only the startup message visible while PIO
continues serving console polls. UART transmission consumes foreground time;
console latch/clock responses remain independent in PIO.

NES/SNES TASD bytes are active-low, MSB-first. The shared codec converts them
to the internal active-high, serial-order state before printing or serializing.
For example, raw NES `FF` releases every button; `EF` presses START only.
Core, input, and output images must be updated together for this corrected
wire representation; the earlier prototype used a nonstandard byte layout.

`playback_reports` counts enqueued playback states (reset at begin), not SPI
transactions. It can lead console consumption by up to four reports.

The current live input-module encoder sends port 0. To use that source instead
of player-1 playback, set `kControllerPort` to 0 and rebuild/flash this module.
Do not accept multiple player ports into the same DATA output.

## Host waveform tests

The standalone CMake project in this directory builds `test_controller_output`
and registers the `controller_output` CTest. Configure it in a separate native
host build directory (not the Pico firmware build), build with CMake Tools,
then run CTest. No Pico SDK is needed for these tests.

The test executes the production PIO source with the firmware's shift
configuration. It covers all 256 NES button combinations, SNES bit order and
tail, repeated polls, held LATCH, interrupted reads, and state updates during a
report. This digital model does not validate pad synchronization, translator
timing, power safety, or a real console.
Playback tests additionally cover the production playback PIO source, ordered
four-entry queues, full-queue rejection, held-LATCH consumption, no advancement
without polling, empty-queue repeats, and refill during an active report.
