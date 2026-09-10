#pragma once

#include <hardware/pio.h>

// PIO program for NES/SNES controller reading
// State machine reads controller data via LATCH and CLOCK
// Generates LATCH pulse, clocks data in, returns 16-bit value

#define NES_PIO_PROGRAM_OFFSET 0
#define NES_PIO_PROGRAM_LENGTH 32

// PIO program instructions
// TODO: Define actual PIO program
const uint16_t nes_pio_program[] = {
    // Placeholder
    0x0000
};
