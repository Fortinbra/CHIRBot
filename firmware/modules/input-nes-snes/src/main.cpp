#include "nes_input_module.hpp"
#include <pico/stdlib.h>
#include <stdio.h>

int main() {
    stdio_init_all();
    // Give USB CDC time to enumerate so startup logging is not lost.
    sleep_ms(2000);

    NESInputModule module;
    module.init();
    
    while (true) {
        module.run();
    }
    
    return 0;
}

// Implementation moved to nes_input_module.cpp

