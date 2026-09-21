#pragma once

#include "st7735_display.hpp"

namespace chirbot::menu {

enum class Mode {
    ControllerInput,
    FilePlayback,
    SdTools,
};

enum class Button {
    None,
    Up,
    Down,
    Select,
};

// Configures the button and SD card-detect GPIOs. Call once at startup.
void init_buttons();

// Debounced, edge-triggered: returns the button that just transitioned to
// pressed, or Button::None if none did since the last call. Call frequently
// (e.g. every ~10 ms) from any menu loop.
Button poll_button();

// Reads the SD breakout's DET pin. See docs/specs/sd-storage.md for the
// assumed polarity (low = card present) and the bring-up verification step.
bool sd_card_present();

// Shows the startup menu (Controller Input / File Playback / SD Tools),
// grays out and blocks entry into File Playback and SD Tools when no SD
// card is detected, and auto-selects Controller Input after a timeout with
// no SELECT press. Always returns (unlike the mode loops below).
Mode run_startup_menu(chirbot::display::St7735Display &display);

// File Playback and SD Tools each run their own internal loop and do not
// return under normal use; the only way out is the physical reset button
// (see docs/specs/menu-system.md "Exiting a mode").
[[noreturn]] void run_file_playback(chirbot::display::St7735Display &display);
[[noreturn]] void run_sd_tools(chirbot::display::St7735Display &display);

}  // namespace chirbot::menu
