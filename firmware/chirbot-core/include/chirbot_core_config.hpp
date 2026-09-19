#pragma once

#include "chirbot/link_spi.h"
#include "hardware/spi.h"

constexpr uint32_t kModuleSpiBaud = CHIRBOT_LINK_SPI_BAUD;
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

// Core config display (ST7735, bit-banged bus; see hardware/PROTOTYPE.md
// "Core display bus").
constexpr uint kDisplayClockPin = 20;
constexpr uint kDisplayMosiPin = 21;
constexpr uint kDisplayChipSelectPin = 22;
constexpr uint kDisplayResetPin = 24;
constexpr uint kDisplayBacklightPin = 25;
constexpr uint kDisplayDataCommandPin = 26;