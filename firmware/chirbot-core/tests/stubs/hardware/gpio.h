#pragma once

#include <cstdint>

constexpr bool GPIO_OUT = true;
void gpio_init(uint32_t pin);
void gpio_set_dir(uint32_t pin, bool output);
void gpio_put(uint32_t pin, bool value);
