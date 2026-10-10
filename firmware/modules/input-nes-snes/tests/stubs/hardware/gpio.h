#pragma once
#include <cstdint>
using uint = unsigned int;
constexpr bool GPIO_OUT = true;
constexpr bool GPIO_IN = false;
void gpio_init(uint pin);
void gpio_set_dir(uint pin, bool output);
void gpio_put(uint pin, bool value);
bool gpio_get(uint pin);
void gpio_disable_pulls(uint pin);
