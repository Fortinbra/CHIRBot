# CHIRBot System Specification

> **Status:** Consolidated implementation reference. The system is in proof
> of concept and hardware bring-up; this document distinguishes accepted
> architecture, demonstrated prototype behavior, planned behavior, and
> unresolved production contracts. It is intended to explain the system
> independently of its source code, not to imply that every capability below
> is implemented or that every interface is production-ready.

## 1. Purpose and scope

CHIRBot is a modular input relay that acquires controller or other supported
device input, routes it to one or more output devices, and supports recording,
playback, configuration, and visualization. Device-specific electrical
interfaces and native protocols belong in replaceable modules. The core owns
discovery, routing, timing coordination, storage, and user-visible system
behavior.

This document describes required behavior and known interface facts for a
clean-room implementation. It is the project-level summary; the linked
specifications and accepted design decisions remain the detailed authorities
for their respective contracts. An explicit unresolved item here or in a
referenced specification is **not** permission to guess a production value.

### 1.1 Requirement maturity

| Label | Meaning |
| --- | --- |
| **Accepted** | Architecture or design decision selected for the system. |
| **Prototype** | Defined and/or demonstrated on bench hardware; not necessarily the production contract. |
| **Draft** | Behavior is proposed or specified but still requires review or verification. |
| **Planned** | Intended capability without an implemented reference behavior. |
| **Open** | A decision is not made. Select and document it before production implementation depends on it. |

The current priority is a repeatable end-to-end hardware path, observable
startup and recovery, and recording electrical/integration findings. The
approximately 1 ms relay objective is a target to measure and refine; it is
not a proof-of-concept pass/fail gate.

## 2. System boundaries and behavior

### 2.1 Components

| Component | Responsibility | Maturity |
| --- | --- | --- |
| Core | Sole module-link controller; discovers and manages modules; routes validated input; provides USB-to-PC connection, display and controls, and microSD services. | RP2350B architecture accepted; bench relay and parts of storage/playback are prototypes. |
| Passive carrier | Connects the core to protocol-specific module slots; distributes buses, per-slot controls, power, and ground. It does not route, interpret TASD, or allow module-to-module communication. | Passive carrier accepted; separate role-specific banks versus universal slots remains open. |
| Dual-role module | One protocol-specific hardware/firmware product that is selected to acquire native input or present native output. | Architecture accepted; existing separate NES input/output prototypes are not yet consolidated. |
| Host application and CLI | Configure the matrix, stream virtual input, visualize state, manage replays, and inspect/validate TASD files. | Planned; application stack and host protocol open. |
| Visualization | Shows live or replayed state on the core display, a future visualization module, or PC software. | Basic core display behavior prototyped; broader visualization planned. |

### 2.2 Data path and ownership

1. A module whose physical role selector was set to input before startup
   samples its native device, translates complete states or events into TASD,
   and exposes pending data to the core.
2. The core validates received link frames and TASD data, identifies their
   source, and routes supported data to configured output modules and other
   sinks such as recording or visualization.
3. A module whose role selector was set to output before startup retains a
   valid state and presents it using the native protocol. The output device
   normally supplies the poll/clock edge; the module must answer that edge
   locally without waiting for another core transaction.
4. Recording stores TASD data. Playback feeds validated recorded input back
   through the output path and advances according to the output device's poll
   behavior, not merely elapsed wall-clock time.

Each protocol-specific module supports exactly one active role at a time. A
per-module physical switch is sampled at startup/reset; moving it while
powered does not change the role until the next reset. Input and output
behavior for a native protocol is documented as one product. Each module
uses a shared native-device connector with the correct role-specific cable or
adapter.

Modules are narrow translators. Routing, remapping policy, recording,
playback, cross-module coordination, and system modes belong to the core.
The passive carrier never makes routing decisions. The core initiates every
module-link transaction; a module never seizes the bus or initiates one.

### 2.3 System capabilities

The architecture is intended to support:

- Input translation from controller families including NES/SNES, USB HID,
  GameCube/N64, and later families such as Genesis/Atari, Bluetooth/wireless,
  and PS/2. Only the capabilities explicitly marked prototype or implemented
  in this document should be treated as available today.
- Output translation to console-native protocols and USB HID, with each
  module responsible for the native electrical and timing behavior in its
  selected role.
- Routing and remapping between supported input and output modules. The
  number of simultaneous routes and conflict policy are not yet defined.
- TASD recording and playback on microSD, plus PC-side inspection and
  management as host tools mature.
- Live state visualization and configuration via the core display, PC
  application, CLI, or a future visualization module.
- USB connection between core and PC for power, configuration, and virtual
  input streaming. The exact USB protocol is open. USB mass storage is a
  separate, mutually exclusive SD Tools mode described in section 7.
- Future device-specific capabilities such as rumble, motion, authentication,
  adaptive controllers, analog controls, and other feedback. Required TASD
  extensions must be proposed upstream; do not silently reuse TASD keys.

## 3. Hardware architecture

### 3.1 Controller boards

**Accepted:** Every active core and input/output module uses the Raspberry Pi
RP2350B and Raspberry Pi Pico SDK. Passive carrier boards do not require an
MCU. RP2350B has 3.3 V GPIO and is **not 5 V tolerant**. Native 5 V device
signals must be translated in the relevant module; 5 V must never be applied
directly to an RP2350B GPIO.

RP2350B has no internal flash; each active board requires external QSPI flash.
The selected board-specific flash size and exact circuit are not fixed here.

### 3.2 Production carrier topology

**Accepted:** the carrier is passive, core-controlled, and supports modular
board-to-board docking. The former topology of four input slots plus four
output slots is documented in
[decision 0003](design/0003-carrier-and-multislot-bus-topology.md), but is no
longer settled for dual-role modules.

Two carrier organizations remain under consideration:

| Candidate | Organization | Consequence to settle |
| --- | --- | --- |
| Role-specific banks | Retain separate input and output SPI banks and their separate CS/IRQ wiring. Dock each switched module in the bank matching its boot-selected role. | Requires the user to match switch setting to bank; define behavior for mismatches and module counts in each bank. |
| Universal slots | Define one slot type that accepts a module in either role. | Determine how role-dependent IRQ and bus wiring are routed, how many slots are supported, and how faults are isolated. |

Neither candidate is selected by this specification. The carrier must not
permit module-to-module communication. Whatever topology is selected, the core
initiates all SPI transactions; only the selected module may drive MISO; CS
inactive levels and reset/power-transition safety must be guaranteed.

Input-role data-ready is a dedicated level-sensitive service request: it
remains asserted while unread data exists and clears only after retrieval or
explicit discard. It does not initiate a transfer. Output-role data is
core-initiated and does not require an IRQ. The contact implementation depends
on the carrier decision.

I2C remains a reserved, disabled 3.3 V management/expansion option; it is not
used for discovery or TASD transport. Pull-up ownership and segment layout
must be specified if it is enabled in a future revision.

The production connector family, contact numbering/orientation, keying,
mechanical outline, power voltage/current/inrush budget, presence detection,
fault isolation, and hot-swap policy are all **open**. Do not treat prototype
connections below as the carrier pinout.

### 3.3 Production logical interface (contact numbers TBD)

The role-independent SPI and safety requirements are fixed; physical contact
assignment and role-dependent carrier routing are open.

| Signal | Direction / sharing | Required behavior |
| --- | --- | --- |
| SCLK | Core to selected bank/slot | Core-generated SPI clock. |
| MOSI | Core to selected module | Core-to-module data. |
| MISO | Selected module to core | High-impedance from all unselected modules, including during reset and power transitions. |
| CS | Core to one module | Active selection; inactive-state pull on carrier. |
| IRQ/alert | Role-dependent | Input role signals pending data; output role does not require it for ordinary output. |
| I2C SDA/SCL | Carrier-topology dependent | Reserved 3.3 V open-drain future bus; unused in current contract. |
| Power and ground | Carrier to each slot / common returns | Voltage, budget, switching, protection, and contact allocation remain open. |

The module role selector has no production GPIO/contact assignment yet. It
must be sampled during startup/reset and may not change the module's role
while running.

### 3.4 Prototype module-link wiring (reference only)

The bench prototype is two independent point-to-point 3.3 V SPI links. These
GPIOs are on different boards and are not production connector contact
numbers.

| Core GPIO | Input-module GPIO | Signal |
| ---: | ---: | --- |
| GP18 | GP18 | SCLK |
| GP19 | GP19 | MOSI |
| GP16 | GP16 | MISO |
| GP17 | GP17 | CS, active low |
| GND | GND | Common reference |

| Core GPIO | Output-module GPIO | Signal |
| ---: | ---: | --- |
| GP14 | GP18 | SCLK |
| GP15 | GP16 | MOSI |
| GP12 | GP19 | MISO |
| GP13 | GP17 | CS, active low |
| GND | GND | Common reference |

Signals cross where needed to connect main-node transmit to subnode receive
and vice versa. Do not join module-link SPI to console 5 V.

The prototype harness currently has no input IRQ. Production input modules
must implement the IRQ behavior above. Prototype modules are attached only
with power off; no production hot-swap behavior has been defined.

### 3.5 NES native controller wiring (prototype reference)

These RJ45/8P8C contact numbers apply only to the NES controller cable adapter,
not the module carrier and not Ethernet. Confirm cable orientation and mapping
by continuity; never infer it from cable color. Do not plug the adapter into
Ethernet equipment.

| NES cable adapter contact | Input-module GPIO | Output-module GPIO | Signal |
| ---: | ---: | ---: | --- |
| 6 | GP2, output to controller | GP2, input from console | LATCH |
| 8 | GP0, output to controller | GP0, input from console | CLOCK |
| 4 | GP4, input from controller | GP4, output to console | DATA0 |
| 5 | Not connected | Not connected | DATA1 |

Input module LATCH/CLOCK are translated from 3.3 V to the native 5 V domain;
input DATA0 is translated from 5 V to 3.3 V. Output module LATCH/CLOCK are
translated from 5 V to 3.3 V; output DATA0 is translated from 3.3 V to 5 V.
The project recommends fixed-direction buffers for clocked push-pull
signals. The assembled input bench uses an electrically unqualified BSS138
translator; successful operation does not qualify it for production.
Keep system and console 5 V rails separate; share ground. Output DATA must be
benign (released buttons) during reset and power transitions.

### 3.6 Core peripheral prototype pinouts

These are current prototype assignments and require verification against the
specific development board. They are not a production core-board connector
specification.

**ST7735 80x160 display, 3.3 V, dedicated PIO-SPI:**

| Display signal | Core GPIO |
| --- | ---: |
| SCL / clock | GP20 |
| SDA / MOSI | GP21 |
| CS | GP22 |
| RES | GP24 |
| BLK | GP25 |
| DC | GP26 |
| VCC / GND | 3V3 / GND |

**microSD breakout, 3.3 V, dedicated PIO-SPI** (hardware SPI0/SPI1 remain
reserved for module links):

| Breakout signal | Core GPIO / rail | Direction |
| --- | --- | --- |
| CLK | GP27 | Core to card |
| DO / MISO | GP28 | Card to core |
| DI / MOSI | GP29 | Core to card |
| CS | GP30 | Core to card, active low |
| DET | GP34 | Card to core; assumed low when card seated, verify board revision |
| 3V / GND | 3V3 / GND | Power / return |

Leave SDIO-only contacts unconnected in SPI mode. The card-detect contact and
polarity are board-revision-dependent; verify before relying on it.

**Startup menu buttons (draft core wiring):**

| Button | Core GPIO | Wiring / behavior |
| --- | ---: | --- |
| UP | GP31 | Normally open to ground; active low with pull-up |
| DOWN | GP32 | Normally open to ground; active low with pull-up |
| SELECT | GP33 | Normally open to ground; active low with pull-up |

### 3.7 Power, safety, and mechanical constraints

- All module-link logic is 3.3 V. Native device voltage translation belongs in
  the device module.
- The carrier must control inactive CS levels and preserve MISO isolation when
  modules are unselected, resetting, or transitioning power.
- Shared SPI bus loading, trace topology, source termination, protection,
  power sequencing, slot fault isolation, and signal-integrity limits require
  electrical design and validation before production.
- No hot-swap capability is implied. Prototype wiring changes are made with
  power off.
- The exact production module-slot/core-to-carrier connectors, contact map,
  module outline, keying, retention, and mounting are open decisions.

## 4. Module discovery and compatibility

Before TASD exchange, the core queries each available module over SPI at
startup, after module reset, and when a slot becomes available. Every module
returns a self-description record containing at least:

- Its role selected by the physical switch and latched at startup/reset.
- Stable vendor and product identifiers.
- Hardware revision and firmware version.
- Minimum and maximum supported SPI link-protocol versions.
- Native protocol or device-family identifier.
- Length-delimited protocol-specific capability set.
- Optional per-instance identifier, such as serial number, where needed.

The core validates that the reported role is compatible with the slot and
routing configuration, selects a mutually supported link version, and checks
capabilities before enabling the data path. Unknown or incompatible modules
remain inactive and are reported to the user. Identity is taken from the
descriptor, not guessed from TASD payloads. Changing the switch while running
does not change the module role; a reset and new discovery are required.

The descriptor byte encoding, numeric registries, discovery commands,
version-negotiation exchange, timeouts, and recovery are open protocol work.

## 5. SPI transport and TASD

### 5.1 Production link rules

The core is the sole SPI controller/main; modules are SPI peripherals/subnodes.
The core initiates discovery, configuration, input reads, and output writes.
Production carrier grouping remains open as defined in section 3. Where SPI
signals are shared, only one slot may be selected at a time and all unselected
MISO outputs must be high-impedance. Input-role data-ready is a service
request only; the core still controls chip select and clocks.

The prototype's framing below is implemented for bench testing, not accepted
as the final production protocol. Production descriptor encoding, scheduling,
error handling, data-ready recovery, timeouts, timing limits, and
conformance vectors remain to be specified.

### 5.2 Prototype SPI frame

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 2 | ASCII magic `CH` |
| 2 | 1 | Protocol version, currently `1` |
| 3 | 1 | Type: `0` empty, `1` live TASD, `2` playback begin, `3` playback state, `4` output status, `5` playback stop, `6` playback config |
| 4 | 4 | Sequence number |
| 8 | 2 | Payload length, 0–50 |
| 10 | 50 | Payload followed by zero padding |
| 60 | 4 | IEEE CRC-32 over bytes 0–59 |

Every transfer is exactly 64 bytes, multi-byte integers are big-endian, SPI is
mode 0, MSB-first, 8-bit words, and the prototype clock is 2 MHz. The core
asserts active-low CS for one frame and initiates one transaction every 1 ms.
Input increments sequence when state changes; the core ignores repeated
sequences and forwards a new validated live frame unchanged to output.

Prototype playback uses type 6 to set a 0–65535 microsecond LATCH-filter
interval before each begin. Types 2 and 3 carry a complete controller-state
TASD document. Begin selects the player-1 playback port/profile, clears the
previous playback queue, and sets released buttons and the filter. State is
acknowledged only after validation and enqueue into the output's four-report
queue; a full queue is not acknowledged, so the core retries the same
sequence. Duplicate accepted commands do not enqueue or reset twice.

The output returns type 4 status on each transaction. Its sequence identifies
the last accepted command; payload byte 0 indicates whether a sequence is
valid and byte 1 reports free queue slots (0–4). Status is prepared before
the current command and therefore acknowledges it on a later transaction.
The core waits at least 1 ms between transactions. Empty type 0 polls status.
At end of file, the core waits for all four slots to be free, ensuring the
last report was latched. Type 5 flushes pending reports, releases buttons,
and returns the output to live mode. A live type-1 frame also returns it to
latest-state mode. Playback errors such as missing/corrupt status or
unacknowledged work with an empty queue time out after three seconds. A
stopped console may leave playback blocked indefinitely; SELECT remains
serviceable.

### 5.3 TASD controller representation

TASD is the common representation for live input, SPI payloads, and stored
recordings. A live prototype report is a complete independently parseable
TASD document: the standard 7-byte TASD header, a `PORT_CONTROLLER`
declaration, and one `INPUT_MOMENT`. The shared Fortinbra/TASD library owns
TASD serialization; implementations must not invent a parallel file format
or silently repurpose TASD keys.

**NES prototype:** controller type `NES_STANDARD` (`0x0101`), live port 0,
one active-low byte, millisecond indexing from input-module boot. The TASD
byte maps bits 7 through 0 to A, B, Select, Start, Up, Down, Left, Right.
`0xFF` means all released; `0xEF` means Start only. An initial state is
emitted at startup, then a report is emitted when the state changes. The
internal implementation may use another representation, but conversion at
the TASD boundary must preserve this defined wire/file meaning.

**SNES output prototype:** accepts `SNES_STANDARD` (`0x0201`) with two
active-low MSB-first bytes. Byte 0 represents B, Y, Select, Start, Up, Down,
Left, Right in bits 7–0. Byte 1 represents A, X, L, R in bits 7–4; bits 3–0
are held at 1. The output serializes B, Y, Select, Start, Up, Down, Left,
Right, A, X, L, R, then inactive/reserved bits. Physical SNES input polling
is not yet implemented.

### 5.4 Prototype output clock behavior

The output device owns its poll clock. For live NES/SNES input, the output
module caches the latest complete valid state. A native LATCH snapshots that
state for the current serial read; updates arriving during a read apply on a
later LATCH, never halfway through a report. With no new input, polls repeat
the latest state.

For NES, LATCH high continuously reloads the report and presents A; LATCH low
freezes it. DATA is stable while CLOCK is low and advances on each rising
CLOCK edge in this order: A, B, Select, Start, Up, Down, Left, Right.
Pressed buttons drive DATA low. After eight bits, DATA stays low. A new LATCH
restarts the report. For SNES, the module shifts sixteen bits and then holds
DATA high. The module begins in an all-buttons-released state and must not
wait for USB logging or another core transaction before answering native
polls.

Playback uses a separate four-report queue. An eligible rising LATCH consumes
one report; holding LATCH high does not consume more. CLOCK shifts the report
without waiting for the core. An empty queue repeats the previous report
(released before the first report). No console poll means no playback
advancement, regardless of elapsed time or video region. LATCH_FILTER enforces
a minimum time between queue advancements: polls during the interval repeat
the same report, and expiration alone does not consume a report. The current
timer may add approximately 3 microseconds to the configured interval.
CLOCK_FILTER is diagnosed as unsupported. Only selected player-1 reports
advance; other ports are skipped. Playback accepts frame-indexed moments and
complete NES/SNES reports, rejects unsupported timebases, partial reports,
and selected-port profile changes. It is single-port playback, not a
multi-controller scheduler.

## 6. Core storage and recordings

The current storage specification defines the following intended v1 behavior:

- A microSD card connects to the core over a dedicated PIO-driven SPI bus;
  do not share hardware SPI0 or SPI1 with the real-time module links.
- Use FAT32 via FatFs for PC-readable files. exFAT is deferred.
- TASD recordings use `.tasd` files. The planned directory is `/recordings/`,
  with firmware-assigned monotonic names such as `rec_00001.tasd`; no RTC is
  required.
- A file contains the TASD header and TASD packets, not a second custom packet
  encoding. Store the same TASD packets used in the live path without
  re-encoding.
- V1 assumes one active input stream/slot per recording; multi-module
  interleaving is not specified.
- The playback read-ahead buffer is bounded at 448 KiB; larger files are
  rejected explicitly, never silently truncated.
- Recording should preserve previously flushed data across power loss or card
  removal and detect an incomplete recording on the next boot. Exact recovery
  and flush guarantees remain open.
- For proof of concept, synchronous storage is acceptable and may add latency
  or jitter. A dual-core design with fixed-size static ring buffers is a
  later performance direction, not a v1 prerequisite.

The card-speed/capacity floor, recording metadata, incomplete-file recovery,
minimum throughput, and production socket/card-detect design remain open.
USB mass-storage access to the card is not part of ordinary core USB
operation; it is a separate exclusive startup mode below.

## 7. Display and on-device controls

The core has a planned/draft startup menu on an ST7735 display. It uses three
momentary, normally-open, active-low buttons with pull-ups:

- **UP / GP31:** move selection up.
- **DOWN / GP32:** move selection down.
- **SELECT / GP33:** confirm.

Buttons are debounced in firmware; the proposed stable-low threshold is
20 ms. Presses are edge-triggered and do not auto-repeat. The menu does not
remember a selection across power cycles.

The defined modes are:

1. **Controller Input** — default and automatic timeout choice. Relay live
   input using the existing matrix path and show controller state.
2. **File Playback** — when a card is present, list `.tasd` files from the
   card root, allow selection, and play decoded controller reports to the
   output while updating the display.
3. **SD Tools** — when a card is present, offer Mass Storage and Format SD.

The proposed startup timeout is 5 seconds. UP/DOWN wrap through selectable
items; SELECT confirms immediately. When card-detect reports no card,
File Playback and SD Tools are visibly disabled and cannot be selected;
Controller Input remains the default and a status message explains the
restriction. Read card-detect again when the menu is redrawn.

Format SD requires an explicit confirmation screen warning that all data will
be erased. UP/DOWN cancels; a 10-second timeout cancels; only SELECT formats
and recreates a FAT32 filesystem.

Mass Storage mode exclusively owns the SD card and USB stack. No matrix relay,
FatFs mount, or playback may run concurrently. Since changing the USB
descriptor requires re-enumeration, the draft design records a reboot marker
and reboots into an MSC-only image path. The display should show that mass
storage is active. Leaving Controller Input, File Playback, or Mass Storage
requires the dedicated hardware reset button; no software back action is
specified for v1.

The display's prototype pinout is in section 3.6. Display coordinates,
fonts, and final core-board hardware are not system-level contracts.

## 8. Host software and external interfaces

The host application is intended to configure modules and routing over USB,
stream virtual input, visualize live input, and manage recordings/replays.
The CLI is intended for scripted configuration and firmware/asset management,
TASD inspection/validation/conversion, and automated streaming/capture tests.
Both are planned; cross-platform support is assumed, but software stack,
USB protocol, message formats, error model, and version negotiation are open.

USB must not be assumed to provide general card file transfer. PC access to
recordings in the current storage design is by reading the FAT32 card through
another reader unless a separate future USB file-access design is accepted.

## 9. Module catalog and current support

| Module family | Input role | Output role | Current maturity |
| --- | --- | --- | --- |
| NES/SNES | Sample native controller protocol and produce TASD input | Emulate standard native controller from TASD | Separate NES input and NES/SNES output prototypes; consolidated module and SNES input are not implemented |
| USB | Read USB controllers, including adaptive devices | Present USB HID modes and future controller/authentication behavior | Planned |
| GameCube/N64 | Acquire Joybus-family controller state | Present Joybus-family controller state | Planned |
| Other families | Add protocol-specific acquisition as needed | Add protocol-specific output as needed | Future |

Module accessories such as Four Score, multitap, mouse, Super Scope, PAL
variants, and other non-standard behavior are not implied by standard
NES/SNES support. Each module must declare its actual protocol and capabilities.

## 10. Reliability, performance, and acceptance

The proof-of-concept baseline is complete when one real hardware input-to-core
to-output path is repeatable, startup/reset/link recovery behavior is
understood, important electrical gotchas are recorded, and focused builds and
tests pass.

For each end-to-end path, validate:

- Correct module selection, frame validation, and state delivery.
- The physical role selector is sampled on startup/reset, discovery reports
  that role, and moving the selector while running does not change behavior.
- Input and output paths are mutually exclusive; reset and power transitions
  never create native-signal contention or an unsafe output state.
- Safe startup/reset behavior and unselected-module MISO high-impedance.
- Link loss, module reset, malformed data, unavailable/incompatible module,
  and recovery are observable and do not produce success-shaped behavior.
- Native signals have correct levels, timing, state ordering, and benign
  behavior during reset, tested with appropriate scope/logic-analyzer
  measurements.
- Recording can be read back, playback reproduces the expected sequence, and
  output advancement tracks native poll edges.
- Latency, jitter, throughput, and electrical margins are measured and
  recorded. Treat the approximately 1 ms total relay target as a
  characterization goal during proof of concept, not a gate.

Production acceptance additionally requires stable module connector and
power contracts; finalized production discovery and SPI protocol; shared-bus
integrity and fault-isolation tests with multiple slots; defined compatible
module behavior; finalized clock-domain and storage recovery policies; and
measured latency/jitter limits.

## 11. Open production decisions

The following must remain visible and must not be filled with invented values:

1. Carrier organization (role-specific banks or universal slots), module
   connector families, physical pinout/contact numbering, board outline,
   keying, mounting, and retention.
2. Module supply voltage, steady-state/inrush budget, protection, power
   switching, presence detection, and hot-swap policy.
3. Production SPI electrical limits, clock rate, bus scheduling/fairness,
   service deadlines, descriptor/command encoding, version negotiation,
   queue reporting, malformed-packet handling, timeout and recovery rules,
   and conformance vectors.
4. Data-ready electrical polarity, pull state, deassertion contract details,
   role-dependent carrier routing, and aggregate input service timing.
5. Supported simultaneous input/output routes, routing conflicts, source
   selection, and multi-module recording.
6. Complete clock/timestamp policy for asynchronous protocols, missed or
   duplicate polls, recording semantics, and source/target clock mismatch.
7. TASD metadata requirements, unsupported packet behavior, incomplete-file
   recovery, and any upstream-required extensions.
8. Minimum microSD speed/capacity and production card-detect implementation.
9. Core USB protocol, host message formats, PC app/CLI implementation, and
   whether future PC access to SD uses mass storage or file transfer.
10. Final hardware, documentation, contribution, and host-support policies
    that are still tracked in project planning.

## 12. Source specifications and decisions

This document consolidates the project-level intent; consult these references
for detailed rationale, draft requirements, and prototype evidence:

- [Architecture](ARCHITECTURE.md)
- [SPI matrix protocol](specs/spi-matrix-protocol.md)
- [Module interface](specs/module-interface.md)
- [TASD usage](specs/tasd-usage.md)
- [Clock domains](specs/clock-domains.md)
- [SD card storage](specs/sd-storage.md)
- [Startup menu and mode selection](specs/menu-system.md)
- [NES-to-UART prototype scenario](specs/nes-to-uart-scenario.md)
- [Design decision 0001: module link](design/0001-module-link-bus-and-connector.md)
- [Design decision 0002: RP2350B MCU](design/0002-mcu-selection-rp2350b.md)
- [Design decision 0003: carrier topology](design/0003-carrier-and-multislot-bus-topology.md)
- [Design decision 0004: dual-role modules](design/0004-dual-role-module-architecture.md)
- [Hardware prototype wiring](../hardware/PROTOTYPE.md)
- [NES module hardware pinouts](../hardware/modules/README.md)
