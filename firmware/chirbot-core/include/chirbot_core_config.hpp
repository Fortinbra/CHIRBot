#pragma once

#include "hardware/spi.h"

constexpr uint32_t kModuleSpiBaud = 2'000'000;
constexpr uint32_t kInputPollIntervalUs = 1'000;

inline spi_inst_t *const kInputSpi = spi0;
constexpr uint kInputMisoPin = 16;
constexpr uint kInputChipSelectPin = 17;
constexpr uint kInputClockPin = 18;
constexpr uint kInputMosiPin = 19;

inline spi_inst_t *const kOutputSpi = spi1;
constexpr uint kOutputMisoPin = 12;
constexpr uint kOutputChipSelectPin = 13;
constexpr uint kOutputClockPin = 14;
constexpr uint kOutputMosiPin = 15;