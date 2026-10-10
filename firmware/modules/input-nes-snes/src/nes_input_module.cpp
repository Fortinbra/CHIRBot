#include "nes_input_module.hpp"
#include <pico/time.h>
#include <stdio.h>

NESInputModule::NESInputModule()
        : tx_frame_{},
            rx_frame_{},
            last_controller_data_(0),
            sequence_(0),
            frame_ready_(false),
            spi_transfers_(0),
            spi_timeouts_(0) {
}

void NESInputModule::init() {
    setup_controller();
    setup_spi();
    
    printf("NES Input Module initialized (physical controller)\n");
}

void NESInputModule::run() {
    const uint16_t buttons = read_controller();
    if (!frame_ready_ || buttons != last_controller_data_) {
        if (encode_tasd_packet(buttons)) {
            last_controller_data_ = buttons;
            frame_ready_ = true;
            printf("[in] seq=%lu buttons=0x%02x transfers=%lu cs_timeouts=%lu\n",
                   (unsigned long)sequence_, buttons,
                   (unsigned long)spi_transfers_, (unsigned long)spi_timeouts_);
        } else {
            printf("[in] failed to encode controller state 0x%02x\n", buttons);
        }
    }

    handle_spi_request();
}
