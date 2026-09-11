#include "nes_input_module.hpp"
#include <hardware/spi.h>
#include <pico/stdlib.h>

namespace {

spi_inst_t *const kCoreSpi = spi0;
constexpr uint kCoreMisoPin = 16;
constexpr uint kCoreChipSelectPin = 17;
constexpr uint kCoreClockPin = 18;
constexpr uint kCoreMosiPin = 19;

}  // namespace

// SPI subnode implementation for module link
// Core drives clock, module responds with TASD packets

void NESInputModule::setup_spi() {
    spi_init(kCoreSpi, 2'000'000);
    spi_set_format(kCoreSpi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    spi_set_slave(kCoreSpi, true);

    gpio_set_function(kCoreMisoPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreChipSelectPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreClockPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreMosiPin, GPIO_FUNC_SPI);
}

void NESInputModule::handle_spi_request() {
    spi_write_read_blocking(kCoreSpi, tx_frame_, rx_frame_, CHIRBOT_LINK_FRAME_SIZE);
}
