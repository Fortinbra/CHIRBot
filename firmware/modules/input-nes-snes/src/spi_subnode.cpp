#include "nes_input_module.hpp"
#include "spi_protocol.hpp"
#include <hardware/spi.h>

// SPI subnode implementation for module link
// Core drives clock, module responds with TASD packets

void NESInputModule::setup_spi() {
    // Configure SPI as subnode (slave)
    // Set up SPI peripheral for module link
    // Handle CS, SCLK, MOSI, MISO
    // TODO: Implement SPI subnode protocol
}

void NESInputModule::handle_spi_request() {
    // Handle SPI requests from core
    // Read TASD packets from core or send controller data
    // TODO: Implement SPI request handling
}
