#include "st7735_display.hpp"

#include "hardware/gpio.h"
#include "pico/stdlib.h"

namespace chirbot::display {

namespace {

// The 132x162 GRAM is bigger than the 80x160 visible glass, so the visible
// area sits at a fixed offset inside it. Since the panel is driven in
// landscape here, CASET (our width/column axis) maps to native rows and
// needs the native row offset, while RASET (our height/row axis) maps to
// native columns and needs the native column offset. Adjust if a different
// panel/tab is wired up.
constexpr int16_t kColumnOffset = 1;
constexpr int16_t kRowOffset = 26;

// Full GRAM extent on the same swapped axes, used to blank the off-screen
// margins so power-on noise outside the visible window is never displayed.
constexpr int16_t kGramWidth = 162;
constexpr int16_t kGramHeight = 132;

// 5x7 glyphs. Each byte is one row, bits 4..0 are columns left to right
// (bit4 = leftmost column).
struct Glyph {
    char character;
    uint8_t rows[7];
};

constexpr Glyph kGlyphs[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'b', {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E}},
    {'o', {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E}},
    {'t', {0x08, 0x08, 0x1F, 0x08, 0x08, 0x08, 0x06}},
};

const uint8_t *glyph_for(char c)
{
    for (const Glyph &glyph : kGlyphs) {
        if (glyph.character == c) {
            return glyph.rows;
        }
    }
    return nullptr;
}

}  // namespace

St7735Display::St7735Display(const St7735Pins &pins) : pins_(pins) {}

void St7735Display::spi_write_byte(uint8_t byte)
{
    for (int8_t bit = 7; bit >= 0; --bit) {
        gpio_put(pins_.mosi, (byte >> bit) & 0x01);
        gpio_put(pins_.clock, 1);
        gpio_put(pins_.clock, 0);
    }
}

void St7735Display::write_command(uint8_t command)
{
    gpio_put(pins_.data_command, 0);
    spi_write_byte(command);
}

void St7735Display::write_data(uint8_t data)
{
    gpio_put(pins_.data_command, 1);
    spi_write_byte(data);
}

void St7735Display::write_data_buffer(const uint8_t *data, size_t length)
{
    gpio_put(pins_.data_command, 1);
    for (size_t index = 0; index < length; ++index) {
        spi_write_byte(data[index]);
    }
}

void St7735Display::init()
{
    gpio_init(pins_.clock);
    gpio_set_dir(pins_.clock, GPIO_OUT);
    gpio_init(pins_.mosi);
    gpio_set_dir(pins_.mosi, GPIO_OUT);
    gpio_init(pins_.chip_select);
    gpio_set_dir(pins_.chip_select, GPIO_OUT);
    gpio_put(pins_.chip_select, 1);
    gpio_init(pins_.data_command);
    gpio_set_dir(pins_.data_command, GPIO_OUT);
    gpio_init(pins_.reset);
    gpio_set_dir(pins_.reset, GPIO_OUT);
    gpio_init(pins_.backlight);
    gpio_set_dir(pins_.backlight, GPIO_OUT);
    gpio_put(pins_.backlight, 1);

    gpio_put(pins_.reset, 1);
    sleep_ms(5);
    gpio_put(pins_.reset, 0);
    sleep_ms(20);
    gpio_put(pins_.reset, 1);
    sleep_ms(150);

    gpio_put(pins_.chip_select, 0);

    write_command(0x01);  // SWRESET
    sleep_ms(150);
    write_command(0x11);  // SLPOUT
    sleep_ms(255);

    write_command(0xB1);  // FRMCTR1
    write_data_buffer((const uint8_t[]){0x01, 0x2C, 0x2D}, 3);
    write_command(0xB2);  // FRMCTR2
    write_data_buffer((const uint8_t[]){0x01, 0x2C, 0x2D}, 3);
    write_command(0xB3);  // FRMCTR3
    write_data_buffer((const uint8_t[]){0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}, 6);

    write_command(0xB4);  // INVCTR
    write_data(0x07);

    write_command(0xC0);  // PWCTR1
    write_data_buffer((const uint8_t[]){0xA2, 0x02, 0x84}, 3);
    write_command(0xC1);  // PWCTR2
    write_data(0xC5);
    write_command(0xC2);  // PWCTR3
    write_data_buffer((const uint8_t[]){0x0A, 0x00}, 2);
    write_command(0xC3);  // PWCTR4
    write_data_buffer((const uint8_t[]){0x8A, 0x2A}, 2);
    write_command(0xC4);  // PWCTR5
    write_data_buffer((const uint8_t[]){0x8A, 0xEE}, 2);
    write_command(0xC5);  // VMCTR1
    write_data(0x0E);

    write_command(0x20);  // INVOFF

    write_command(0x36);  // MADCTL: landscape orientation
    write_data(0x60);

    write_command(0x3A);  // COLMOD: 16 bits/pixel
    write_data(0x05);

    write_command(0x29);  // DISPON
    sleep_ms(100);

    gpio_put(pins_.chip_select, 1);

    clear_gram();
}

void St7735Display::set_address_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    set_address_window_raw(static_cast<int16_t>(x0 + kColumnOffset),
                           static_cast<int16_t>(y0 + kRowOffset),
                           static_cast<int16_t>(x1 + kColumnOffset),
                           static_cast<int16_t>(y1 + kRowOffset));
}

void St7735Display::set_address_window_raw(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    write_command(0x2A);  // CASET
    write_data_buffer((const uint8_t[]){
        0x00, static_cast<uint8_t>(x0),
        0x00, static_cast<uint8_t>(x1)}, 4);

    write_command(0x2B);  // RASET
    write_data_buffer((const uint8_t[]){
        0x00, static_cast<uint8_t>(y0),
        0x00, static_cast<uint8_t>(y1)}, 4);

    write_command(0x2C);  // RAMWR
}

void St7735Display::clear_gram()
{
    gpio_put(pins_.chip_select, 0);
    set_address_window_raw(0, 0, kGramWidth - 1, kGramHeight - 1);

    gpio_put(pins_.data_command, 1);
    for (int32_t index = 0; index < kGramWidth * kGramHeight; ++index) {
        spi_write_byte(0x00);
        spi_write_byte(0x00);
    }

    gpio_put(pins_.chip_select, 1);
}

void St7735Display::fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color565)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    gpio_put(pins_.chip_select, 0);
    set_address_window(x, y, static_cast<int16_t>(x + width - 1), static_cast<int16_t>(y + height - 1));

    const uint8_t high_byte = static_cast<uint8_t>(color565 >> 8);
    const uint8_t low_byte = static_cast<uint8_t>(color565 & 0xFF);

    gpio_put(pins_.data_command, 1);
    const int32_t pixel_count = static_cast<int32_t>(width) * static_cast<int32_t>(height);
    for (int32_t index = 0; index < pixel_count; ++index) {
        spi_write_byte(high_byte);
        spi_write_byte(low_byte);
    }

    gpio_put(pins_.chip_select, 1);
}

void St7735Display::fill_screen(uint16_t color565)
{
    fill_rect(0, 0, kWidth, kHeight, color565);
}

void St7735Display::draw_char(int16_t x, int16_t y, char c, uint16_t color565, uint8_t scale)
{
    const uint8_t *rows = glyph_for(c);
    if (rows == nullptr) {
        return;
    }

    for (uint8_t row = 0; row < 7; ++row) {
        for (uint8_t column = 0; column < 5; ++column) {
            const bool pixel_on = (rows[row] & (0x10 >> column)) != 0;
            if (!pixel_on) {
                continue;
            }
            fill_rect(static_cast<int16_t>(x + column * scale),
                      static_cast<int16_t>(y + row * scale),
                      scale, scale, color565);
        }
    }
}

void St7735Display::draw_text(int16_t x, int16_t y, const char *text, uint16_t color565, uint8_t scale)
{
    const int16_t advance = static_cast<int16_t>(6 * scale);
    int16_t cursor_x = x;
    for (const char *character = text; *character != '\0'; ++character) {
        draw_char(cursor_x, y, *character, color565, scale);
        cursor_x = static_cast<int16_t>(cursor_x + advance);
    }
}

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

uint16_t rainbow565(float hue_degrees)
{
    float hue = hue_degrees;
    while (hue < 0.0f) {
        hue += 360.0f;
    }
    while (hue >= 360.0f) {
        hue -= 360.0f;
    }

    const float h = hue / 60.0f;
    const int sector = static_cast<int>(h) % 6;
    const float fraction = h - static_cast<float>(static_cast<int>(h));
    const uint8_t rising = static_cast<uint8_t>(255.0f * fraction);
    const uint8_t falling = static_cast<uint8_t>(255.0f * (1.0f - fraction));

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    switch (sector) {
        case 0: r = 255; g = rising; b = 0; break;
        case 1: r = falling; g = 255; b = 0; break;
        case 2: r = 0; g = 255; b = rising; break;
        case 3: r = 0; g = falling; b = 255; break;
        case 4: r = rising; g = 0; b = 255; break;
        default: r = 255; g = 0; b = falling; break;
    }

    return rgb565(r, g, b);
}

}  // namespace chirbot::display
