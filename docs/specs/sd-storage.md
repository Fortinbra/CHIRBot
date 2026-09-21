# SD card storage (TASD recording and playback)

**Issue:** TBD
**Title:** microSD storage on the CHIRBot core: TASD recording/playback
**Status:** Draft

> **Status:** Draft. The project is in the proof-of-concept stage: the goal
> for this feature right now is to prove SD storage, recording, and playback
> can be integrated into the core at all, end to end. Design questions below
> (bus, filesystem, on-disk format, naming, initial scope) are resolved enough
> to start building; see "Decisions recap". Latency and throughput are kept
> in mind throughout but are **not** treated as blocking criteria yet — they
> get measured and revisited once the integration works. This follows
> [docs/SPEC_TEMPLATE.md](../SPEC_TEMPLATE.md) and should move to
> `Review`/`Accepted` once the remaining Open Questions are closed, at which
> point the resolved decisions should be split out into `docs/design/` ADRs
> the same way [0001](../design/0001-module-link-bus-and-connector.md)-
> [0003](../design/0003-carrier-and-multislot-bus-topology.md) capture earlier
> decisions.

## Problem Statement

The core currently has no persistent storage. [docs/ARCHITECTURE.md](../ARCHITECTURE.md)
already commits to on-device microSD for recordings, and the roadmap's
vertical slice explicitly calls for "recording and playback through the same
path" ([docs/ROADMAP.md](../ROADMAP.md) step 2). The selected part is the
[Adafruit Micro SD SPI or SDIO Card Breakout Board (PID 4682)](https://www.adafruit.com/product/4682) —
a **3V-only** breakout with no level shifting or onboard regulator, which is a
direct electrical match for the RP2350B's native 3.3 V logic.

The card's filesystem must be one a PC can read directly (e.g. by physically
removing the card or, later, some other reader path) so recordings are
portable without a custom tool — that requirement drives the filesystem
choice below, but it is **not** a requirement to expose the card over the
core's USB connection. The core's existing USB link to the PC is reserved for
another purpose to be defined later and is out of scope here.

We need to design, end to end:

1. A physical/electrical connection from the core RP2350B to the SD breakout.
2. Firmware that can mount a filesystem on the card and read/write files.
3. A filesystem choice that a PC can also read/write directly, independent of
   the core (e.g. with the card removed).
4. Recording of live TASD input traffic to `.tasd` files on the card.
5. Playback of a stored `.tasd` file back into the SPI matrix as if it were a
   live input module.

This is one of the more involved core features (block storage + filesystem +
real-time recording/playback), so this document exists to get every major
question in front of the team before writing firmware.

## Constraints from the existing architecture

These are not open questions — they come from already-accepted decisions and
already-consumed hardware and must shape the design below:

- **Both RP2350B hardware SPI peripherals are already committed.**
  [`chirbot_core_config.hpp`](../../firmware/chirbot-core/include/chirbot_core_config.hpp)
  assigns `spi0` to the input-module link and `spi1` to the output-module
  link, and both links carry latency-sensitive real-time TASD traffic ("Core
  is the main node" — see the SPI link gotchas captured during input/output
  bring-up). **The SD card cannot use either hardware SPI block without
  contending with the matrix links.** The core config display already sets
  the precedent for this: it is bit-banged (PIO or plain GPIO), not one of the
  two hardware SPI instances. The SD interface uses a dedicated PIO-SPI state
  machine driven by DMA (see Hardware > Bus and pin assignment below), not
  either hardware SPI block.
- **The core owns USB to the PC already** for "power, matrix configuration,
  and virtual input streaming" (ARCHITECTURE.md §1), and that link is
  reserved for another purpose to be scoped later. This feature does **not**
  add a mass-storage/file-transfer role to the core's USB connection; PC
  access to card contents (if any, in this phase) happens by other means
  (e.g. removing the card).
- **TASD is the storage format**, encoded/decoded via the vendored
  [Fortinbra/TASD library](../../firmware/TASD/include/tasd.h)
  (`firmware/TASD`), wrapped by `chirbot-common` rather than reimplemented
  (ARCHITECTURE.md §2.3 item 4). Recording/playback file I/O is new code in
  `chirbot-common` or `chirbot-core`; TASD framing itself is not.
- **Output device owns the clock domain** (ARCHITECTURE.md §2.3 item 5).
  Playback must be paced to the output module's poll edge, not written out as
  fast as the card allows — this is the same timing model
  [docs/specs/clock-domains.md](clock-domains.md) is already meant to define.
- The prototype three-node bring-up explicitly plans for "PGA2350 core +
  microSD + USB" ([hardware/PROTOTYPE.md](../../hardware/PROTOTYPE.md)), so
  this feature lands on the same development board already used for the core,
  not a separate fixture.

## Design Goals

- Read and write raw blocks on a microSD card from the RP2350B core over a
  dedicated PIO-SPI bus (the breakout also supports native SDIO, deferred —
  see Hardware > Bus and pin assignment).
- Mount a standard filesystem (FAT32 for v1) so files are portable to any PC
  without a custom tool — this is the reason to prefer a standard filesystem
  over a bespoke on-card layout, independent of how (or whether) the core
  ever exposes it over USB.
- Record live TASD input traffic from the SPI matrix to a `.tasd` file.
  Keep the ~1 ms relay latency budget in mind while designing this, but at
  the PoC stage the goal is proving the recording path integrates end to end;
  measuring and closing any latency gap is follow-up work, not a gate here.
- Play back a stored `.tasd` file into the matrix so it behaves like a live
  input module to the rest of the system (output modules, visualization).
- Survive power loss or card removal during a recording without corrupting
  previously-flushed data.
- Keep the storage/filesystem layer decoupled from matrix routing logic, the
  same separation of concerns already used for SPI link vs. TASD codec
  (ARCHITECTURE.md §2.4 firmware layering).

## Hardware

### Selected part

[Adafruit Micro SD SPI or SDIO Card Breakout Board, PID 4682](https://www.adafruit.com/product/4682):

- No level shifters, no onboard 3.3 V regulator — **3.3 V logic and power
  only**. This matches the RP2350B directly; do not power it from 5 V.
- Breakout exposes: `3V`, `GND`, `CLK`, `DO` (card DO / SPI MISO), `DI` (card
  DI / SPI MOSI), `CS`, and `DAT2`/other SDIO data lines for optional 4-bit
  SDIO. SPI mode only needs `3V, GND, CLK, DO, DI, CS`.
- Confirm silkscreen labels against the current revision before wiring —
  Adafruit has revised this board's silkscreen at least once (see product
  page revision history).

### Prototype SPI pinout

The prototype uses the next contiguous free GPIO block on the core RP2350B.
These pins are reserved for the dedicated PIO-SPI state machine and must not
be reassigned to either module link or the display:

| Color | Breakout signal | RP2350B core pin | Direction from core |
| --- | --- | --- | --- |
| Red | `3V` | `3V3` | Power |
| Black | `GND` | `GND` | Return |
| Yellow | `CLK` | `GP27` | Core to card |
| White | `DO` / SPI MISO | `GP28` | Card to core |
| Green | `DI` / SPI MOSI | `GP29` | Core to card |
| Purple | `CS` | `GP30` | Core to card, active low |
| White (flagged with tape) | `DET` | `GP34` | Card to core |

Leave `DAT2` and the other SDIO data lines unconnected in SPI mode. **Decision:
use the breakout's `DET` pin for card presence instead of probing via
initialization/I/O.** The board in bench use has a `DET` pin wired to the
socket's mechanical card-detect switch; this was not expected from the PID
4682 product page referenced above when this document was first written, so
confirm the label and behavior against the specific board in hand before
relying on it — some revisions/vendors omit it, and switch polarity is not
standardized. Assumed behavior, to verify during bring-up with a multimeter
continuity check: `DET` reads low (switch closed) when a card is fully
seated, and high (open, pulled up by the core's internal pull-up) when no
card is present or the card is only partially inserted.

### Bus and pin assignment (decision)

Candidates considered, roughly in order of expected effort:

1. **PIO-based SPI** dedicated to the SD card, mirroring how the ST7735
   display bus is already bit-banged/PIO-driven off the two hardware SPI
   blocks. Keeps SPI0/SPI1 timing pristine for the matrix.
2. **Software (bit-banged, non-PIO) SPI**, simplest to bring up but likely too
   slow for acceptable recording/playback throughput and CPU overhead —
   probably only acceptable as a bring-up fallback.
3. **Native SDIO via PIO** for higher throughput, at the cost of needing
   4 data lines routed and a PIO SDIO driver instead of a simple SPI driver.

**Decision: option 1, a dedicated PIO-SPI state machine, as the target
design.** This keeps both hardware SPI blocks untouched (a hardware
conflict, not a tunable choice — the matrix links already own SPI0/SPI1), so
it's the only option that avoids contending with the matrix regardless of
performance goals:

- It leaves both hardware SPI blocks untouched, so the matrix links keep
  their existing timing guarantees with zero contention risk.
- For the PoC, a straightforward blocking PIO-SPI driver (no DMA yet) is
  enough to prove mount/read/write/record/playback integration end to end.
  Driving it from DMA instead of blocking the requesting core is a
  performance follow-up once the integration is proven, not a prerequisite —
  keep it in mind, but don't block the PoC on it.
- Bit-banged (non-PIO) SPI remains an acceptable quick bring-up fallback if a
  working PIO-SPI driver isn't ready yet; it can be swapped out later without
  changing anything above the block-driver layer.
- RP2350B has 12 PIO state machines across two PIO blocks (ARCHITECTURE.md
  §2.3), so one more dedicated to SD is a small fraction of the budget already
  established by the display bus precedent.
- Native SDIO is deferred: it needs 4 data lines routed (vs. 4 total pins for
  SPI already: CLK/DO/DI/CS) and a less mature PIO SDIO driver. Only revisit
  if SPI throughput is later measured to be a real bottleneck.

This should still be written up as a short `docs/design/000N-*.md` ADR once
GPIO/PIO assignment across the whole core (matrix bit-bang display, module
links, and now SD) is finalized, the same way the SPI matrix bus topology was
decided in
[design/0003](../design/0003-carrier-and-multislot-bus-topology.md).

### Physical placement

`hardware/chirbot-core-hw/README.md` already lists a "microSD slot" as an
expected element of the core board. This document's job is to pin down its
electrical connection; connector/slot part selection is a hardware-team
decision (mechanical card-edge micro-SD socket vs. this breakout as a
prototype stand-in).

## API / Protocol

### Firmware layers

```
┌───────────────────────────────────────────┐
│ Recording / playback service (chirbot-core)│
│  • start/stop recording, start/stop replay │
├───────────────────────────────────────────┤
│ TASD file I/O (chirbot-common)             │
│  • .tasd file read/write on top of a       │
│    filesystem, using firmware/TASD codec   │
├───────────────────────────────────────────┤
│ Filesystem (e.g. FatFs)                    │
├───────────────────────────────────────────┤
│ SD block driver (SPI or SDIO, PIO-based)   │
├───────────────────────────────────────────┤
│ Pico SDK (RP2350B)                         │
└───────────────────────────────────────────┘
```

This mirrors the layering already documented in ARCHITECTURE.md §2.4 —
storage is a new vertical slice through the same style of stack, not a
special case.

### Candidate libraries (decision)

**Decision: FatFs (ChaN), FAT32 only for v1**, using
`no-OS-FatFS-SD-SPI-RPi-Pico` (carlk3) as the reference integration but with
its SPI backend replaced by the PIO-SPI driver above instead of a hardware
SPI block.

- FatFs is the de facto standard for microcontrollers, has a small, predictable
  footprint, and its GPL-alternative license option is compatible with a
  vendored, unmodified-core-with-thin-wrapper approach.
- **Skip exFAT for v1.** It adds license surface (the non-GPL FatFs option)
  and cluster/allocation-bitmap overhead for no real benefit here: TASD
  recordings are input-event streams, not large media files, and FAT32's 4 GiB
  per-file / up to 2 TiB volume limits are far beyond any realistic session
  length. Revisit only if a future requirement produces multi-gigabyte files.
- FAT32's fixed cluster size lets recording/playback align writes/reads to
  whole clusters, avoiding read-modify-write overhead on the card — call this
  out explicitly in the implementation once a cluster size is chosen for the
  target card capacities.
- Vendor it the same way as TASD (git submodule/pinned commit under
  `firmware/`, thin wrapper in `chirbot-common`) per ARCHITECTURE.md §3, and
  give it the same code-quality/test review TASD itself still needs
  (ARCHITECTURE.md §5).

### Function Signatures / Module Boundaries

The current bring-up uses the FatFs R0.15 sources bundled with the installed
Pico SDK. The SD disk-I/O callbacks use the dedicated GPIO SPI block driver;
normal startup mounts the existing volume and lists its root directory. The
destructive formatter is never called automatically: entering the exact
console command `FORMAT SD` followed by Enter erases the card and creates a
new FAT32 filesystem, then remounts it. USB CDC and UART0 both use the same
console input path.

Example shape only — exact API to be finalized during implementation:

```c
// chirbot-common: SD block device + filesystem lifecycle
bool sd_storage_init(void);
bool sd_storage_mounted(void);
void sd_storage_unmount(void);

// chirbot-common: TASD file recording
typedef struct chirbot_recording chirbot_recording_t;

bool chirbot_recording_start(chirbot_recording_t *rec, const char *path);
bool chirbot_recording_write_moment(chirbot_recording_t *rec,
                                     const tasd_packet_t *packet);
bool chirbot_recording_stop(chirbot_recording_t *rec);

// chirbot-common: TASD file playback
typedef struct chirbot_playback chirbot_playback_t;

bool chirbot_playback_open(chirbot_playback_t *pb, const char *path);
bool chirbot_playback_next_moment(chirbot_playback_t *pb,
                                    tasd_packet_t *out_packet,
                                    uint32_t *out_due_time_us);
void chirbot_playback_close(chirbot_playback_t *pb);
```

### PC filesystem access (out of scope here)

The core's USB connection to the PC is not used for filesystem/mass-storage
access in this feature — it is reserved for a different, not-yet-defined
purpose. The only requirement this document places on the filesystem choice
is that it be a standard one (FAT32/exFAT) so a PC can read/write it directly
by other means (e.g. the card physically removed and read in a normal SD
reader), without needing a bespoke tool to parse a custom layout.

If a PC-over-USB path to the same files is wanted later, it should be scoped
as its own follow-up once the core's USB roadmap is defined, and would need
its own design/ADR (composite device interfaces, exclusive-access handling
with concurrent recording, etc.) at that time — it is intentionally not
designed here.

### Concurrency and buffering (future optimization, not required for the PoC)

Once latency needs to be closed, the target design is to **split storage
across the RP2350B's two cores**: core 0 keeps servicing the real-time SPI
matrix exactly as it does today, core 1 owns the filesystem, the PIO-SPI
driver, and all SD card I/O, and the two communicate only through fixed-size,
statically-allocated (never stack-allocated — large buffers as file-scope/
static storage per prior embedded lessons here) lock-free
single-producer/single-consumer ring buffers of TASD packets. That would give:

- **Recording:** core 0 pushes each captured TASD packet onto a record ring
  buffer as it already relays it to the output link; core 1 pops packets and
  writes them to the open file, so recording never adds blocking SD latency
  to the matrix's relay path.
- **Playback:** core 1 reads ahead from the file into a playback ring buffer;
  core 0 pops the next due packet and feeds it into the matrix exactly at the
  output module's poll edge, independent of SD read latency/jitter.

**For the PoC, this is not required.** A single-core, synchronous
implementation (record/playback calls block on SD I/O directly) is
acceptable to prove the feature integrates — it may add latency or jitter to
the matrix while it does SD I/O, and that's an expected, known trade-off at
this stage, not a defect to fix now. Keep the interfaces between the matrix
service and the storage code narrow (see Function Signatures below) so the
dual-core/ring-buffer version can be dropped in later without changing
callers, matching the layering already used elsewhere
(ARCHITECTURE.md §2.4).

## Data Structures

```c
// On-card layout (decision: sequence-numbered, no PC round-trip required)
// /recordings/rec_00001.tasd, rec_00002.tasd, ...
//   Standard 7-byte TASD header (see docs/specs/tasd-usage.md)
//   followed by a stream of TASD packets in file order
// /recordings/.next_seq                - next sequence number (small counter file)
```

**Decision: firmware auto-names files from a persisted monotonic sequence
counter** (`rec_NNNNN.tasd`), not a timestamp. The RP2350B has no
battery-backed RTC, so a wall-clock name would be meaningless after a power
cycle unless a PC or user supplies one; a counter is simpler, cheaper to
generate (no clock dependency, no string/date formatting on the hot path),
and collision-free by construction. User-supplied names/metadata can be layered
on top later (e.g. via the on-device display/controls or a future PC tool)
without changing the underlying file.

Recording/playback reuses the TASD packet types already defined for the live
link ([docs/specs/tasd-usage.md](tasd-usage.md)) rather than inventing a
parallel on-disk format. **Decision:** the file stores the exact TASD packets
already produced for the live SPI transport, unmodified — this is also the
most efficient option, since it lets the record ring buffer (above) carry the
same bytes the matrix already relays with no extra re-encoding step. This
resolves, for CHIRBot's usage, the "boundary between TASD file packets and the
live SPI transport envelope" question tracked as open in
[docs/specs/tasd-usage.md](tasd-usage.md); that document should be updated to
reference this decision once reviewed.

```c
// Recording state machine
typedef enum {
    REC_IDLE,
    REC_ACTIVE,
    REC_ERROR
} chirbot_recording_state_t;

// Playback state machine
typedef enum {
    PB_IDLE,
    PB_PLAYING,
    PB_PAUSED,
    PB_DONE,
    PB_ERROR
} chirbot_playback_state_t;
```

## Validation & Acceptance Criteria

These focus on functional integration, matching the current PoC stage.
Latency/throughput should be measured once these pass, but are not gating
criteria yet.

- [ ] Core firmware detects card insertion/absence and reports mount status
      (e.g., on the config display)
- [ ] Core can mount a FAT32-formatted card and list/create/delete files
- [ ] A file written by the core can be read back byte-identical after a power
      cycle
- [ ] A file written by the core is readable on a PC when the card is read by
      other means (e.g. removed and placed in a standard SD reader), without
      corruption
- [ ] A file written by a PC (via the same means) is readable by the core
- [ ] Core can record live SPI-matrix TASD input traffic to a `.tasd` file for
      the duration of a session without dropping or reordering packets
- [ ] A recording started and stopped mid-session produces a valid, playable
      `.tasd` file (no dangling/incomplete state)
- [ ] Sudden power loss during recording does not corrupt previously-flushed
      data, and the partial file is either valid up to the last flush or
      cleanly detected as incomplete on next boot
- [ ] Core can play back a stored `.tasd` file into the matrix, and a
      connected output module reproduces the original input sequence with
      correct timing relative to its poll edge
- [ ] End-to-end input-to-output relay latency with recording/playback active
      is measured and recorded (baseline for later optimization) — not
      required to hit the ~1 ms target yet

## Decisions recap

Resolved by this document (see the relevant section above for rationale):

| Question | Decision |
| --- | --- |
| SD bus | Dedicated PIO-SPI state machine (not hardware SPI0/1, which are already committed); blocking driver for the PoC, DMA is a later optimization, SDIO deferred |
| Filesystem | FatFs, FAT32 only for v1 (exFAT deferred) |
| Third-party SD driver | `no-OS-FatFS-SD-SPI-RPi-Pico` as reference, PIO-SPI backend, vendored like TASD |
| Recording/playback concurrency | Single-core, synchronous for the PoC; dual-core + lock-free ring buffers is the target design once latency needs closing |
| File naming | Firmware-assigned monotonic sequence counter (`rec_NNNNN.tasd`), no RTC dependency |
| On-disk packet format | Identical TASD packets already used on the live SPI transport, no re-encoding |
| Initial recording scope | Single input slot only for v1 (explicit assumption, not just a default) |
| Minimum card requirements | Reject cards below a minimum speed class/capacity at mount time; no floor proposed yet (see Open Questions) |

## Open Questions

Still genuinely open — deliberately deferred or requiring input outside this
document's scope:

- **Future PC access over USB:** once the core's USB connection's other
  purpose is defined, will there be a need to also reach recordings over USB,
  and if so via MSC or a vendor file-transfer protocol? Deliberately deferred,
  not designed in this document.
- **Minimum card speed class to standardize on:** no floor has been proposed
  yet. This needs to be set from measured PIO-SPI throughput against the
  ~1 ms budget rather than picked arbitrarily; until then, detect card
  capability via CSD/OCR at mount time and reject cards below whatever floor
  is eventually chosen rather than accept degraded performance silently.
- **Multi-module recording sessions are a future decision, not made here.**
  For now, v1 explicitly assumes a single input slot is populated and
  recorded/played back at a time — there is exactly one active TASD stream
  per file, and no interleaving/merging of multiple input modules. Revisit
  once multi-slot routing itself is designed
  ([docs/ROADMAP.md](../ROADMAP.md) step 4); this assumption, not just the
  routing question, will need to be re-examined at that point.
- Incomplete-file recovery semantics on next boot (beyond "don't corrupt
  previously-flushed data") and any additional file metadata are TASD-usage
  decisions that should be finalized together with
  [docs/specs/tasd-usage.md](tasd-usage.md).
- Physical microSD socket part selection for production hardware (this
  document only covers the Adafruit breakout used for prototyping); the
  prototype's `DET` pin (see "Prototype SPI pinout" above) covers card
  presence for now, but production socket part selection is still a
  hardware-team decision.

## Implementation Notes

- Prototype bring-up should use the Adafruit PID 4682 breakout wired directly
  to the core's RP2350B dev board, consistent with how the input/output
  module links and display were first bench-wired
  ([hardware/PROTOTYPE.md](../../hardware/PROTOTYPE.md)) before any custom PCB
  exists.
- Follow the existing wiring-documentation convention (see
  `hardware/PROTOTYPE.md` and the project's wiring-color notes) once pins are
  assigned, so the SD harness is traceable the same way the SPI module links
  and display bus are.
- Given the ~1 ms relay budget matters long-term, measure end-to-end latency
  once recording/playback is integrated, but treat the first working version
  as a functional PoC, not a latency-optimized implementation — optimize
  (DMA, dual-core split) only after integration is proven and a real
  measurement shows where the time goes.
- This feature should land after the shared specs in
  [docs/ROADMAP.md](../ROADMAP.md) step 1 stabilize (module interface, SPI
  matrix protocol, TASD usage, clock domains), since recording/playback
  timing depends directly on the clock-domain model.
