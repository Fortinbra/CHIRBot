#include "nes_input_module.hpp"
#include "chirbot/link_spi.h"
#include "chirbot/spi_subnode.h"
#include <hardware/spi.h>
#include <pico/stdlib.h>

namespace {

// The pinmux fixes these roles: GP16 is SPI0 RX and GP19 is SPI0 TX. A subnode
// receives MOSI on RX and drives MISO from TX, so the harness must cross
// (core GP19 -> module GP16, module GP19 -> core GP16). GP19 cannot receive
// and GP16 cannot drive, regardless of what the wiring is labelled.
spi_inst_t *const kCoreSpi = spi0;
constexpr uint kCoreMosiPin = 16;
constexpr uint kCoreChipSelectPin = 17;
constexpr uint kCoreClockPin = 18;
constexpr uint kCoreMisoPin = 19;

}  // namespace

// SPI subnode implementation for module link
// Core drives clock, module responds with TASD packets

void NESInputModule::setup_spi() {
    spi_init(kCoreSpi, CHIRBOT_LINK_SPI_BAUD);
    spi_set_format(kCoreSpi, CHIRBOT_LINK_SPI_DATA_BITS, CHIRBOT_LINK_SPI_CPOL,
                   CHIRBOT_LINK_SPI_CPHA, CHIRBOT_LINK_SPI_ORDER);
    spi_set_slave(kCoreSpi, true);

    gpio_set_function(kCoreMisoPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreChipSelectPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreClockPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreMosiPin, GPIO_FUNC_SPI);
}

void NESInputModule::handle_spi_request() {
    if (!chirbot_spi_subnode_transfer(kCoreSpi, kCoreChipSelectPin, tx_frame_,
                                      rx_frame_, CHIRBOT_LINK_FRAME_SIZE)) {
        ++spi_timeouts_;
    }
    ++spi_transfers_;
}
