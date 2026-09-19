#pragma once

#include <cstdint>

#include "st7735_display.hpp"

namespace chirbot::display {

// Pass as duration_ms to cycle forever and never return.
constexpr uint32_t kIndefinite = 0;

// Draws `text` centered on `display`, cycling every letter through the
// rainbow for `duration_ms` before returning. Runs forever if duration_ms is
// kIndefinite.
void show_rainbow_splash(St7735Display &display, const char *text, uint32_t duration_ms);

}  // namespace chirbot::display
