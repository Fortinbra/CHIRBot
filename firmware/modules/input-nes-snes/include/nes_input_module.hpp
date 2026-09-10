#pragma once

#include <stdint.h>

class NESInputModule {
public:
    NESInputModule();
    void init();
    void run();
    
private:
    // PIO state for NES/SNES protocol
    void setup_pio();
    uint16_t read_controller();
    
    // SPI subnode communication
    void setup_spi();
    void handle_spi_request();
    
    // TASD packet handling
    void encode_tasd_packet(uint16_t controller_data);
};
