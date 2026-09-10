#pragma once

#include <stdint.h>
#include <tasd.h>

// TASD packet wrapper for NES/SNES controller data
// Maps controller state to TASD format

class TASDPacket {
public:
    static void encode_controller_data(uint16_t data, uint8_t controller_type);
    static void encode_input_chunk(uint16_t data, uint32_t timestamp);
    
private:
    static uint8_t buffer[256];
};
