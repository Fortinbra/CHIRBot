#include "nes_input_module.hpp"
#include <stdio.h>

NESInputModule::NESInputModule() {
}

void NESInputModule::init() {
    setup_pio();
    setup_spi();
    
    printf("NES Input Module initialized\n");
}

void NESInputModule::run() {
    // Main loop
    uint16_t data = read_controller();
    // TODO: Encode to TASD and send via SPI
}
