# chirbot-core

> **Status:** First SPI-to-UART relay prototype implemented. Hardware design is
> not finalized; current work prioritizes end-to-end bring-up and finding
> hardware/integration gotchas over final latency optimization.

Firmware for the CHIRBot core board (RP2350B). Responsibilities:

- **SPI matrix switch (main)** — routes TASD-packetized input between sources
- **microSD storage** — mounts and lists the FAT filesystem; type `FORMAT SD`
  followed by Enter on the USB/UART console to erase the card and create a
  fresh FAT32 filesystem
  (input module, PC stream, microSD playback) and sinks (output module,
  visualization, microSD recording); ~1 ms is an eventual target, not the
  current PoC gate
- **USB to PC** — device power, matrix configuration, virtual input streaming,
  microSD exposure as mass storage
- **microSD storage** — TASD macro/run recording and playback
- **Config display + simple controls** — on-device recording/playback/settings
- **Module management** — initiate all SPI transactions; query docked module
  descriptors; validate type, version, and capabilities; handle input
  data-ready signals; manage module power and configuration

Depends on [chirbot-common](../chirbot-common/) and the
[TASD library](../TASD/). See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).

The current prototype image polls an input module over SPI0, validates and
decodes its TASD frame, prints every packet over UART0, and forwards new frames
over SPI1. It does not yet implement production discovery or input data-ready
signaling.
Controller Input forwards live TASD port-0 reports unchanged; the output module
routes those to its console connector independently of movie playback's port-1
selector. The input module samples a physical NES controller and supplies
initial and changed states.
See the [NES-to-UART scenario](../../docs/specs/nes-to-uart-scenario.md) for
wiring, UF2 paths, and expected output.

## Playback diagnostics

Core diagnostic output is available on both UART0 and the core's own USB CDC
serial port. USB logging does not wait for a terminal; open the port before
selecting a file to capture `Playback:` errors. An unread USB terminal cannot
block playback.

A Pico probe connected to the output module's UART reports output-module logs,
not core playback errors. Its startup message is `NES/SNES output module ready`.
Use the separate serial port enumerated by the core's USB connection to diagnose
file parsing or output acknowledgement failures.

Playback sends the file's LATCH-filter configuration to the output before
beginning. For `3922M.tasd`, `Playback: latch_filter_us=8000` should appear.
Progress includes elapsed milliseconds; the output's `playback_reports` counter
counts queued reports rather than status-poll transactions. Both images must be
updated together. CLOCK-filter metadata is currently logged as unsupported.

Movie and live TASD bytes are active-low, MSB-first. They are converted to
internal active-high serial-order masks before visualization and output.
The movie's initial `EF` is START only; `FF` is released, not all buttons.
Update input, core, and output images together for this corrected TASD layout.

## NES button visualization

Live relay and file playback display all eight buttons from a single player-1
report in two rows: A, B, SELECT, START / UP, DOWN, LEFT, RIGHT. Pressed buttons
have a green background; released buttons remain visible in grey. Simultaneous
presses are not reduced to the first set bit or combined across frames.

Only changed button cells are redrawn, using one display write window per cell;
identical reports do not redraw. The display shows the latest accepted/queued
report, not an acknowledgement that the console has read its serial bits.

The installed 80x160 panel uses landscape column addresses 0..159 (no
horizontal offset) and row offset 26. Both cell writes and the startup clear
stay within that column range to avoid right-edge row wrapping.
