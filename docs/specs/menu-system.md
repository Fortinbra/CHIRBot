# On-device startup menu and mode selection

**Issue:** TBD
**Title:** Three-button startup menu selecting Controller Input, File Playback, or SD Tools
**Status:** Draft

---

## Problem Statement

The core currently boots directly into a single hardcoded behavior: relay
live input from the input module to the output module, decode TASD packets,
and mirror button state on the config display. There is no way to choose a
different behavior (e.g. play back a recorded `.tasd` file instead of live
input, or expose the SD card as USB mass storage) without reflashing firmware
with a different `CHIRBOT_ENABLE_USB_MSC` build.

We need an on-device menu, driven by physical buttons and the existing ST7735
config display, that lets the user pick a mode at startup, while still
booting into a safe default automatically if nobody is present to choose
(e.g. the core is powered from a console with nobody at the bench).

## Design Goals

- Three momentary push-buttons (UP, DOWN, SELECT) are enough input for a
  simple vertical menu; no analog input or long-press gestures are needed.
- A visible countdown/timeout auto-selects a default mode if no button is
  pressed, so the device is still usable unattended.
- Exactly one mode is active at a time; switching modes is a deliberate,
  menu-driven action, not automatic.
- Reuse existing subsystems rather than duplicating them:
  - Controller Input reuses the existing SPI matrix relay + TASD decode +
    display path already in `main.cpp`.
  - File Playback reuses the existing FatFs mount/read code in
    [`sd_storage.cpp`](../../firmware/chirbot-core/src/sd_storage.cpp) and the
    TASD reader already used for the live path.
  - SD Tools' Format option reuses the existing
    `chirbot::sd::format_card()`.
  - SD Tools' Mass Storage option reuses the existing USB MSC code path that
    is currently gated behind the `CHIRBOT_ENABLE_USB_MSC` CMake option.
- Mass Storage mode must fully own the SD card and USB stack; no concurrent
  FatFs mount, matrix relay, or playback may run while it is active.
- Debounce buttons in firmware; no hardware debounce components are assumed.

## Hardware

### Buttons

Three momentary, normally-open push-buttons, each wired from a core GPIO to
the shared ground already run to the button bank. Firmware enables the
internal pull-up on each pin, so the idle state reads high and a press pulls
the pin low (active-low, no external resistor required).

These are the next free contiguous GPIOs after the microSD bus
(`GP27`–`GP30`, see [sd-storage.md](sd-storage.md)):

| Button | Core pin | Wire color | Notes |
| --- | --- | --- | --- |
| UP | `GP31` | White | Moves the highlighted menu item up |
| DOWN | `GP32` | White (flagged with one wrap of tape) | Moves the highlighted menu item down |
| SELECT | `GP33` | White (flagged with two wraps of tape) | Confirms the highlighted item |

White is the project's existing color for a "peripheral-to-core" or secondary
control signal (see `hardware/PROTOTYPE.md` "Wiring color reference"); reusing
it for three independent button signals and flagging two of the three wires
with tape follows the same precedent already used for the display's `RES`/
`BLK` lines. Ground is out of scope here since it is already run to the
button bank per the request that started this spec.

Verify `GP31`/`GP32`/`GP33` are unused and accessible on the specific PGA2350
board against the
[Pimoroni pinout diagram](https://cdn.shopify.com/s/files/1/0174/1800/files/pga2350_pinout_diagram.pdf?v=1723124465)
before wiring, the same way the display bus pins were checked.

### Debounce

No hardware debounce (RC or capacitor) is assumed. Firmware debounces by
requiring a pin to read stable low for a minimum hold time (proposed 20 ms)
before registering a press, and treats a press as a single edge-triggered
event (auto-repeat is not needed for a 3-item menu).

## API / Protocol

### Modes

```c
typedef enum {
    CHIRBOT_MODE_CONTROLLER_INPUT,  // default
    CHIRBOT_MODE_FILE_PLAYBACK,
    CHIRBOT_MODE_SD_TOOLS,
} chirbot_mode_t;
```

`CHIRBOT_MODE_CONTROLLER_INPUT` is the default selection and the timeout
fallback: it matches today's always-on behavior, so a core with nobody at the
bench (e.g. installed behind a console) keeps working exactly as it does now.
**Decision: Controller Input is always the hardcoded default and the
timeout fallback; the menu does not persist or remember a last-selected
mode across power cycles.** This keeps the unattended/console-installed case
predictable regardless of whatever was last selected on the bench.

### Startup menu

```c
typedef enum {
    MENU_BUTTON_NONE,
    MENU_BUTTON_UP,
    MENU_BUTTON_DOWN,
    MENU_BUTTON_SELECT,
} menu_button_t;

// Debounced, edge-triggered: returns the button that just transitioned to
// pressed, or MENU_BUTTON_NONE if none did since the last call.
menu_button_t menu_poll_button(void);

// Shows the mode list on `display`, highlighting `default_mode`, and a
// visible countdown. Returns the user's choice, or `default_mode` if
// `timeout_ms` elapses with no SELECT press.
chirbot_mode_t menu_run_startup(chirbot::display::St7735Display &display,
                                chirbot_mode_t default_mode,
                                uint32_t timeout_ms);
```

Proposed default timeout: 5000 ms. UP/DOWN move the highlight (wrapping
top-to-bottom); SELECT confirms immediately and skips the remaining timeout.

### File Playback mode

```c
// chirbot-common or chirbot-core: list .tasd files for the playback picker
typedef struct {
    char name[64];
    uint32_t size_bytes;
} chirbot_tasd_file_entry_t;

// Fills `out_entries` (capacity `max_entries`) with root-directory .tasd
// files; returns the number found (may be less than the number on disk if
// max_entries is exceeded).
size_t sd_list_tasd_files(chirbot_tasd_file_entry_t *out_entries, size_t max_entries);

// Opens the selected file and plays it into the output link, decoding TASD
// input-moment packets and pacing them the same way the live output module
// polling already paces relay per docs/specs/clock-domains.md.
bool chirbot_playback_open(const char *name);
bool chirbot_playback_next_moment(tasd_pkt_input_moment_t *out_event,
                                  uint32_t *out_due_time_us);
void chirbot_playback_close(void);
```

Function shapes are illustrative only, matching the existing convention in
[sd-storage.md](sd-storage.md#function-signatures--module-boundaries); exact
signatures are finalized during implementation. Playback mode:

1. Mounts the FAT filesystem (reusing `chirbot::sd::mount_filesystem()`).
2. Lists `.tasd` files with `sd_list_tasd_files()` and shows them in a
   scrollable menu using the same UP/DOWN/SELECT buttons.
3. On selection, reads and decodes TASD input-moment packets, updates the
   display the same way `update_display_from_event()` does today, and sends
   the decoded button state to the output module link on each due moment.
4. Playback runs until the file ends or the user exits via the dedicated
   reset button (see "Exiting a mode" below); no in-firmware "back" gesture
   is needed for v1.

### SD card presence (decision)

**Decision: use the SD breakout's `DET` pin (see
[sd-storage.md](sd-storage.md#prototype-spi-pinout), `GP34`) to gate menu
items instead of probing the card via initialization/I/O, and gray out at the
top level, not just the sub-items.** Whenever `DET` reads "no card present"
at the time the startup menu is drawn, **File Playback and SD Tools are
grayed out and not enterable at all** — the menu does not descend into SD
Tools only to show disabled Mass Storage/Format SD items. Controller Input is
drawn normally and highlighted, with a visible status line (e.g. "No SD
card — Controller Input only") making it unambiguous that it is the only
usable option. The menu re-reads `DET` each time it is (re)drawn, so
inserting a card and reopening the menu clears the grayed-out state without a
reset.

This avoids the cost and failure modes of probing the card (SPI
initialization, `CMD0`/`ACMD41` handshake) just to decide whether a menu item
should be selectable, and matches the pattern already used for buttons: a
simple debounced GPIO read.

### SD Tools mode

Two items, only reachable when `DET` indicates a card is present: **Mass
Storage** and **Format SD**.

**Mass Storage:** while active, no other core function may run — no matrix
relay, no FatFs mount, no playback. TinyUSB does not support swapping a
device's descriptor set (CDC console vs. MSC disk) at runtime without
re-enumerating, so the proposed mechanism is:

1. On selecting Mass Storage, write a marker value to a watchdog scratch
   register (persists across a soft reset on RP2350) and call
   `watchdog_reboot()`.
2. At the top of `main()`, check the scratch register before any other
   initialization. If the marker is set, clear it and run only the existing
   MSC-only path (today's `CHIRBOT_ENABLE_USB_MSC` code), skipping the
   matrix/display/menu init entirely.
3. The display should still show a static "Mass storage active — reset to
   exit" message so the user has on-device confirmation, since the console/
   UART are unavailable in this mode by design (see the earlier SD-over-USB
   work).
4. Exiting requires the dedicated reset button (see "Exiting a mode" below);
   there is no software exit for v1.

**Format SD:** shows an explicit warning screen before doing anything
destructive:

```text
ERASE SD CARD?
This deletes ALL data.
SELECT = confirm
UP/DOWN = cancel
(auto-cancel in 10s)
```

Only an explicit SELECT on this warning screen calls
`chirbot::sd::format_card()`; the timeout on this screen cancels (opposite of
the startup menu's timeout, which proceeds) since silence must never trigger
a destructive action.

### Exiting a mode (decision)

**Decision: for v1, the only way to leave Controller Input, File Playback, or
Mass Storage and return to the startup menu is the dedicated hardware reset
button** (momentarily shorts `RUN` to `GND`). No mode implements its own
software "back"/exit control:

- This is the same mechanism already used to recover the board today, so it
  needs no new wiring, firmware state, or button gesture.
- It sidesteps the awkward case of Mass Storage mode, where the console/UART
  are unavailable by design and a software exit would need its own signaling
  path back to normal firmware.
- A future revision may add a friendlier in-menu exit once there is a
  concrete need for it; it is not required for the proof-of-concept.

## Data Structures

```c
typedef enum {
    MENU_STATE_STARTUP,
    MENU_STATE_CONTROLLER_INPUT,
    MENU_STATE_FILE_PICKER,
    MENU_STATE_FILE_PLAYBACK,
    MENU_STATE_SD_TOOLS,
    MENU_STATE_FORMAT_CONFIRM,
} chirbot_menu_state_t;
```

## Validation & Acceptance Criteria

- [ ] Pressing UP/DOWN moves the highlighted item and wraps at the top/bottom
      of the list
- [ ] Pressing SELECT immediately enters the highlighted mode and cancels the
      startup countdown
- [ ] No button press within the timeout enters Controller Input automatically
- [ ] Controller Input mode behaves identically to the current always-on
      relay/display behavior
- [ ] Controller Input is selected automatically on timeout and is never
      grayed out, even with no SD card present
- [ ] File Playback and SD Tools are grayed out/unselectable at the
      top-level menu whenever `DET` indicates no card is present — SD Tools
      is not enterable at all in that state, and a status line makes
      Controller Input's exclusivity obvious
- [ ] File Playback, Mass Storage, and Format SD all become selectable again
      after the menu is redrawn with a card inserted
- [ ] File Playback mode lists every `.tasd` file in the SD root directory
- [ ] Selecting a file plays back its decoded button states to the output
      module and updates the display in the same visual style as live input
- [ ] Entering Mass Storage mode disables the matrix relay, FatFs mount, and
      playback for the remainder of that power cycle
- [ ] Selecting Format SD without confirming (timeout, or pressing UP/DOWN)
      never erases the card
- [ ] Confirming Format SD erases and recreates a FAT32 filesystem, reusing
      the existing formatter, and returns to SD Tools afterward
- [ ] A button held down (not released) does not repeat-trigger menu actions
      every polling cycle

## Open Questions

None open at this time; see "SD card presence (decision)" above for the
resolved gray-out behavior.

## Implementation Notes

- Debounce, edge-detection, and the startup countdown should live in one
  small `menu.cpp`/`menu.hpp` module in `firmware/chirbot-core`, separate from
  `main.cpp`'s existing relay loop, mirroring the separation already used for
  `sd_probe`/`sd_storage`.
- Reuse `chirbot::display::St7735Display` drawing primitives already present
  (`fill_rect`, `draw_text`) for the list/highlight/countdown UI; no new
  display driver work should be needed.
- The watchdog-scratch-register technique for persisting the Mass Storage
  request across `watchdog_reboot()` is a standard RP2040/RP2350 pattern
  (scratch registers survive a software reset) and needs no new hardware.

## References

- [SD card storage](sd-storage.md) — FatFs mount, format, and TASD file read
  functions this spec reuses
- [Clock domains](clock-domains.md) — output-module poll-edge pacing that
  File Playback mode must follow
- [Wiring color reference](../../hardware/PROTOTYPE.md) — existing bench wire
  color conventions this spec's button table follows
- [CHIRBot Architecture](../ARCHITECTURE.md) — system overview
