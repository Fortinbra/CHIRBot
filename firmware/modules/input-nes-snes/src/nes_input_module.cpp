#include "nes_input_module.hpp"
#include <stdio.h>

NESInputModule::NESInputModule()
        : tx_frame_{},
            rx_frame_{},
            last_controller_data_(0),
            sequence_(0),
            frame_ready_(false) {
}

void NESInputModule::init() {
    setup_controller();
    setup_spi();
    
    printf("NES Input Module initialized\n");
}

void NESInputModule::run() {
    const uint16_t data = read_controller();
    if (!frame_ready_ || data != last_controller_data_) {
        encode_tasd_packet(data);
        last_controller_data_ = data;
        frame_ready_ = true;
    }

    handle_spi_request();
}
