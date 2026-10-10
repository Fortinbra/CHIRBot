#pragma once

#include <cstdint>

#include "hardware/spi.h"

inline spi_inst_t *const kCoreSpi = spi0;
constexpr uint kCoreMosiPin = 16;
constexpr uint kCoreChipSelectPin = 17;
constexpr uint kCoreClockPin = 18;
constexpr uint kCoreMisoPin = 19;

constexpr uint kConsoleLatchPin = 2;
constexpr uint kConsoleClockPin = 0;
constexpr uint kConsoleDataPin = 4;
constexpr uint kConsoleOutputEnablePin = 10;
// Reserved former TXS OE connection; BSS138 has no enable input.