#include "nes_input_module.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>

namespace {

constexpr uint kControllerLatchPin = 2;
constexpr uint kControllerClockPin = 3;
constexpr uint kControllerDataPin = 4;
constexpr uint32_t kLatchPulseUs = 12;
constexpr uint32_t kClockHalfPeriodUs = 6;

}  // namespace

void NESInputModule::setup_controller() {
    gpio_init(kControllerLatchPin);
    gpio_set_dir(kControllerLatchPin, GPIO_OUT);
    gpio_put(kControllerLatchPin, 0);

    gpio_init(kControllerClockPin);
    gpio_set_dir(kControllerClockPin, GPIO_OUT);
    gpio_put(kControllerClockPin, 0);

    gpio_init(kControllerDataPin);
    gpio_set_dir(kControllerDataPin, GPIO_IN);
}

uint16_t NESInputModule::read_controller() {
    uint8_t raw_state = 0;

    gpio_put(kControllerLatchPin, 1);
    sleep_us(kLatchPulseUs);
    gpio_put(kControllerLatchPin, 0);
    sleep_us(kClockHalfPeriodUs);

    for (uint8_t bit = 0; bit < 8u; ++bit) {
        if (gpio_get(kControllerDataPin)) {
            raw_state |= (uint8_t)(1u << bit);
        }

        gpio_put(kControllerClockPin, 1);
        sleep_us(kClockHalfPeriodUs);
        gpio_put(kControllerClockPin, 0);
        sleep_us(kClockHalfPeriodUs);
    }

    return (uint8_t)~raw_state;
}
