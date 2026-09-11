#pragma once

#include <stdint.h>

#include "chirbot/link_protocol.h"

class NESInputModule {
public:
    NESInputModule();
    void init();
    void run();
    
private:
    // Native NES controller protocol
    void setup_controller();
    uint16_t read_controller();
    
    // SPI subnode communication
    void setup_spi();
    void handle_spi_request();
    
    // TASD packet handling
    void encode_tasd_packet(uint16_t controller_data);

    uint8_t tx_frame_[CHIRBOT_LINK_FRAME_SIZE];
    uint8_t rx_frame_[CHIRBOT_LINK_FRAME_SIZE];
    uint16_t last_controller_data_;
    uint32_t sequence_;
    bool frame_ready_;
};
