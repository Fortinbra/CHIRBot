#include "nes_input_module.hpp"
#include <stdio.h>

int main() {
    NESInputModule module;
    module.init();
    
    while (true) {
        module.run();
    }
    
    return 0;
}

// Implementation moved to nes_input_module.cpp

