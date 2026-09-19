#pragma once

#include <cstddef>
#include <cstdint>

// Bit-banged driver for the core config display (ST7735, 80x160, see
// hardware/PROTOTYPE.md "Core display bus"). SPI0/SPI1 are already committed
// to the module links, so this bus is driven with plain GPIO instead of a
// hardware SPI or PIO instance.
//
// NOTE: CASET/RASET offsets below assume a common 160x80 "red tab" ST7735
// module. Re-check against the actual panel's datasheet during bring-up;
// some tabs need different column/row offsets.

namespace chirbot::display {

struct St7735Pins {
    uint32_t clock;
    uint32_t mosi;
    uint32_t chip_select;
    uint32_t data_command;
    uint32_t reset;
    uint32_t backlight;
};

class St7735Display {
public:
    static constexpr int16_t kWidth = 160;
    static constexpr int16_t kHeight = 80;

    explicit St7735Display(const St7735Pins &pins);

    void init();
    void fill_screen(uint16_t color565);
    void fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color565);
    void draw_char(int16_t x, int16_t y, char c, uint16_t color565, uint8_t scale);
    void draw_text(int16_t x, int16_t y, const char *text, uint16_t color565, uint8_t scale);

private:
    void write_command(uint8_t command);
    void write_data(uint8_t data);
    void write_data_buffer(const uint8_t *data, size_t length);
    void set_address_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void spi_write_byte(uint8_t byte);

    St7735Pins pins_;
};

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
uint16_t rainbow565(float hue_degrees);

}  // namespace chirbot::display
