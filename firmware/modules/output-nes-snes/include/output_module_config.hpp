#pragma once

#include "hardware/spi.h"

inline spi_inst_t *const kCoreSpi = spi0;
constexpr uint kCoreMosiPin = 16;
constexpr uint kCoreChipSelectPin = 17;
constexpr uint kCoreClockPin = 18;
constexpr uint kCoreMisoPin = 19;

constexpr uint kConsoleLatchPin = 2;
constexpr uint kConsoleClockPin = 3;
constexpr uint kConsoleDataPin = 4;

static_assert(kConsoleClockPin == kConsoleLatchPin + 1,
			  "console clock must immediately follow latch for the PIO pin mapping");