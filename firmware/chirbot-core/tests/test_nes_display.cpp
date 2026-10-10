#include "st7735_display.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

constexpr chirbot::display::St7735Pins kPins{0, 1, 2, 3, 4, 5};
std::vector<uint8_t> bytes;
std::array<uint16_t, 160 * 80> pixels{};
bool selected = false;
bool data = false;
bool mosi = false;
uint8_t byte = 0;
uint8_t bits = 0;
uint8_t command = 0;
std::vector<uint8_t> arguments;
uint16_t x0 = 0, x1 = 0, y0 = 0, y1 = 0, x = 0, y = 0;
bool high_pixel_byte = true;
uint16_t color = 0;

constexpr uint8_t kRightColumnGlyphs[2][5][7] = {
    {
        {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e},
        {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
        {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},
        {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11},
        {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}
    },
    {
        {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11},
        {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f},
        {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f},
        {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},
        {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}
    }
};

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void receive(uint8_t value)
{
    bytes.push_back(value);
    if (!data) {
        command = value;
        arguments.clear();
        high_pixel_byte = true;
        if (command == 0x2c) {
            x = x0;
            y = y0;
        }
    } else if (command == 0x2a || command == 0x2b) {
        arguments.push_back(value);
        if (arguments.size() == 4u) {
            const uint16_t start = static_cast<uint16_t>((arguments[0] << 8) | arguments[1]);
            const uint16_t end = static_cast<uint16_t>((arguments[2] << 8) | arguments[3]);
            if (command == 0x2a) {
                check(start <= end && end < 160u,
                      "landscape column window must not extend past address 159");
                x0 = start;
                x1 = end;
            } else {
                y0 = start;
                y1 = end;
            }
        }
    } else if (command == 0x2c) {
        if (high_pixel_byte) {
            color = static_cast<uint16_t>(value << 8);
        } else {
            color = static_cast<uint16_t>(color | value);
            check(x < 160u && y < 132u, "display writes stay within address bounds");
            if (y >= 26u && y <= 105u) {
                pixels[(y - 26u) * 160u + x] = color;
            }
            if (++x > x1) {
                x = x0;
                ++y;
            }
            check(y <= y1 + 1u, "window pixel count");
        }
        high_pixel_byte = !high_pixel_byte;
    }
}

} // namespace

void gpio_init(uint32_t) {}
void gpio_set_dir(uint32_t, bool) {}
void sleep_ms(uint32_t) {}

void gpio_put(uint32_t pin, bool value)
{
    if (pin == kPins.chip_select) {
        selected = !value;
    } else if (pin == kPins.data_command) {
        data = value;
    } else if (pin == kPins.mosi) {
        mosi = value;
    } else if (pin == kPins.clock && value && selected) {
        byte = static_cast<uint8_t>((byte << 1) | static_cast<uint8_t>(mosi));
        if (++bits == 8u) {
            receive(byte);
            byte = 0;
            bits = 0;
        }
    }
}

int main()
{
    using namespace chirbot::display;
    St7735Display display(kPins);
    display.init();
    uint8_t previous = 0;
    for (uint16_t state = 0; state <= 255u; ++state) {
        bytes.clear();
        draw_nes_button_state(display, static_cast<uint8_t>(state));
        unsigned changed = 0;
        for (uint8_t bit = 0; bit < 8u; ++bit) {
            const bool pressed = (state & (1u << bit)) != 0u;
            const size_t cell_x = (bit % 4u) * 40u;
            const size_t cell_y = 29u + (bit / 4u) * 12u;
            const uint16_t background = pressed ? rgb565(0, 100, 0) : rgb565(0, 0, 0);
            const uint16_t foreground = pressed ? rgb565(255, 255, 255) : rgb565(100, 100, 100);
            bool have_glyph = false;
            for (size_t row = 0; row < 9u; ++row) {
                for (size_t column = 0; column < 40u; ++column) {
                    const uint16_t pixel = pixels[(cell_y + row) * 160u + cell_x + column];
                    check(pixel == foreground || pixel == background, "complete per-frame cell colors");
                    have_glyph = have_glyph || pixel == foreground;
                    if (row == 0u || row == 8u) {
                        check(pixel == background, "all simultaneous bits and releases are reflected");
                    }
                    if (bit == 3u || bit == 7u) {
                        bool ink = false;
                        if (row >= 1u && row <= 7u && column >= 5u && column < 34u) {
                            const size_t text_column = column - 5u;
                            if (text_column % 6u < 5u) {
                                ink = (kRightColumnGlyphs[bit / 4u][text_column / 6u][row - 1u] &
                                       (0x10u >> (text_column % 6u))) != 0u;
                            }
                        }
                        check(pixel == (ink ? foreground : background),
                              "START and RIGHT match exact glyph pixels without row shearing");
                    }
                }
            }
            check(have_glyph, "every button label remains visible");
            changed += ((state ^ previous) & (1u << bit)) != 0u ? 1u : 0u;
        }
        if (state != 0u) {
            check(bytes.size() == changed * (11u + 40u * 9u * 2u),
                  "only changed cells use a single write window each");
        }
        bytes.clear();
        draw_nes_button_state(display, static_cast<uint8_t>(state));
        check(bytes.empty(), "identical frame does not redraw");
        previous = static_cast<uint8_t>(state);
    }
    display.fill_screen(rgb565(0, 0, 0));
    bytes.clear();
    draw_nes_button_state(display, 255u);
    check(bytes.size() > 160u * 80u * 2u, "menu clear invalidates button view");
    draw_nes_button_state(display, 0u);
    for (uint8_t bit = 0; bit < 8u; ++bit) {
        check(pixels[(29u + (bit / 4u) * 12u) * 160u + (bit % 4u) * 40u] == 0u,
              "releasing all eight buttons clears every highlight");
    }
    std::cout << "NES display: all 256 masks, releases, redraw bounds and caching passed\n";
}
