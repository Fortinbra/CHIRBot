#include "splash_screen.hpp"

#include <cmath>
#include <cstddef>

#include "pico/stdlib.h"

namespace chirbot::display {

namespace {

constexpr uint8_t kScale = 4;
constexpr int16_t kCharWidth = 5 * kScale;
constexpr int16_t kCharSpacing = kScale;
constexpr int16_t kCharHeight = 7 * kScale;
constexpr float kDegreesPerSecond = 180.0f;
constexpr float kHueStepPerChar = 360.0f / 7.0f;
constexpr uint32_t kFramePeriodMs = 33;

}  // namespace

void show_rainbow_splash(St7735Display &display, const char *text, uint32_t duration_ms)
{
    size_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    if (length == 0) {
        return;
    }

    const int16_t total_width = static_cast<int16_t>(
        length * kCharWidth + (length - 1) * kCharSpacing);
    const int16_t start_x = static_cast<int16_t>((St7735Display::kWidth - total_width) / 2);
    const int16_t start_y = static_cast<int16_t>((St7735Display::kHeight - kCharHeight) / 2);

    display.fill_screen(rgb565(0, 0, 0));

    const absolute_time_t start_time = get_absolute_time();
    while (duration_ms == kIndefinite ||
           absolute_time_diff_us(start_time, get_absolute_time()) <
               static_cast<int64_t>(duration_ms) * 1000) {
        const float elapsed_seconds =
            static_cast<float>(absolute_time_diff_us(start_time, get_absolute_time())) /
            1'000'000.0f;
        const float base_hue = elapsed_seconds * kDegreesPerSecond;

        int16_t cursor_x = start_x;
        for (size_t index = 0; index < length; ++index) {
            const float hue = base_hue + static_cast<float>(index) * kHueStepPerChar;
            display.draw_char(cursor_x, start_y, text[index], rainbow565(hue), kScale);
            cursor_x = static_cast<int16_t>(cursor_x + kCharWidth + kCharSpacing);
        }

        sleep_ms(kFramePeriodMs);
    }
}

}  // namespace chirbot::display
