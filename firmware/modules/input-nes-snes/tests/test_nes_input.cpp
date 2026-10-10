#include "nes_input_module.hpp"
#include "hardware/gpio.h"

#include <cstdint>
#include <cstdio>
#include <stdexcept>

namespace {
bool latch = false;
bool clock_high = true;
bool data_input = false;
uint8_t pressed = 0;
unsigned serial_bit = 0;
unsigned reads = 0;
unsigned pulses = 0;
unsigned frames = 0;
unsigned transfers = 0;
uint16_t encoded = 0;
unsigned disabled_pulls = 0;
uint64_t elapsed = 0;

void check(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}
}

void gpio_init(uint) {}
void gpio_set_dir(uint pin, bool output)
{
    if (pin == 4u) {
        check(!output, "DATA must be input");
        data_input = true;
    } else {
        check(output, "CLOCK/LATCH must be outputs");
    }
}
void gpio_disable_pulls(uint pin) { disabled_pulls |= 1u << pin; }
void gpio_put(uint pin, bool value)
{
    if (pin == 2u) {
        if (value) {
            check(clock_high, "CLOCK must be high during latch");
            serial_bit = 0;
        }
        latch = value;
    } else if (pin == 0u) {
        if (value && !clock_high) {
            check(!latch, "CLOCK pulse must follow latch release");
            ++serial_bit;
            ++pulses;
        }
        clock_high = value;
    }
}
bool gpio_get(uint pin)
{
    check(pin == 4u && data_input && !latch && clock_high,
          "Sample DATA while clock high after latch");
    check(serial_bit < 8u, "Read exactly eight controller bits");
    ++reads;
    return (pressed & (1u << serial_bit)) == 0u;
}
void sleep_us(uint64_t us)
{
    check(us == (latch ? 12u : 6u), "Native latch/clock timing");
    elapsed += us;
}

void NESInputModule::setup_spi() {}
void NESInputModule::handle_spi_request() { ++transfers; }
bool NESInputModule::encode_tasd_packet(uint16_t value)
{
    encoded = value;
    ++frames;
    ++sequence_;
    return true;
}

int main()
{
    try {
        NESInputModule module;
        module.init();
        check(disabled_pulls == ((1u << 0u) | (1u << 2u) | (1u << 4u)),
              "BSS pins use external pull-ups");
        for (unsigned mask = 0; mask <= UINT8_MAX; ++mask) {
            pressed = static_cast<uint8_t>(mask);
            const unsigned before = frames;
            const uint64_t time_before = elapsed;
            reads = pulses = 0;
            module.run();
            check(encoded == mask && frames == before + 1u,
                  "Initial/changed state retains all buttons in serial order");
            check(reads == 8u && pulses == 8u && !latch && clock_high,
                  "Eight samples/pulses and idle restoration");
            check(elapsed - time_before == 114u, "Read uses bounded native timing");
            module.run();
            check(frames == before + 1u, "Unchanged state does not emit a new frame");
        }
        pressed = 0;
        module.run();
        check(encoded == 0u && frames == 257u, "Button release emits released state");
        check(transfers == 513u, "SPI served on every iteration");
        std::puts("NES physical input tests passed");
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
    return 0;
}
