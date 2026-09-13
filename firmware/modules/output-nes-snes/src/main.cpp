#include "output_module_config.hpp"

#include <cstdio>

#include "chirbot/controller_tasd.h"
#include "chirbot/link_protocol.h"
#include "controller_output.pio.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

namespace {

enum class ControllerProfile {
    Nes,
    Snes
};

PIO controller_pio = pio0;
uint controller_sm = 0;
uint nes_program_offset = 0;
uint snes_program_offset = 0;
ControllerProfile active_profile = ControllerProfile::Nes;
uint32_t wire_state = 0xFFFFFFFFu;

void init_core_spi()
{
    spi_init(kCoreSpi, 2'000'000);
    spi_set_format(kCoreSpi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    spi_set_slave(kCoreSpi, true);

    gpio_set_function(kCoreMosiPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreChipSelectPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreClockPin, GPIO_FUNC_SPI);
    gpio_set_function(kCoreMisoPin, GPIO_FUNC_SPI);
}

void start_controller_profile(ControllerProfile profile)
{
    pio_sm_set_enabled(controller_pio, controller_sm, false);
    pio_sm_clear_fifos(controller_pio, controller_sm);
    pio_sm_restart(controller_pio, controller_sm);

    uint program_offset;
    pio_sm_config config;
    if (profile == ControllerProfile::Snes) {
        program_offset = snes_program_offset;
        config = snes_controller_output_program_get_default_config(program_offset);
    } else {
        program_offset = nes_program_offset;
        config = nes_controller_output_program_get_default_config(program_offset);
    }

    sm_config_set_out_pins(&config, kConsoleDataPin, 1);
    sm_config_set_set_pins(&config, kConsoleDataPin, 1);
    sm_config_set_in_pins(&config, kConsoleLatchPin);
    sm_config_set_out_shift(&config, true, false, 32);

    pio_gpio_init(controller_pio, kConsoleDataPin);
    pio_sm_set_consecutive_pindirs(controller_pio, controller_sm,
                                   kConsoleDataPin, 1, true);
    pio_sm_init(controller_pio, controller_sm, program_offset, &config);
    pio_sm_set_pins_with_mask(controller_pio, controller_sm,
                              1u << kConsoleDataPin, 1u << kConsoleDataPin);
    pio_sm_put_blocking(controller_pio, controller_sm, wire_state);
    pio_sm_set_enabled(controller_pio, controller_sm, true);
    active_profile = profile;
}

}  // namespace

int main()
{
    stdio_init_all();
    init_core_spi();

    gpio_init(kConsoleLatchPin);
    gpio_set_dir(kConsoleLatchPin, GPIO_IN);
    gpio_init(kConsoleClockPin);
    gpio_set_dir(kConsoleClockPin, GPIO_IN);

    controller_sm = pio_claim_unused_sm(controller_pio, true);
    nes_program_offset = pio_add_program(controller_pio, &nes_controller_output_program);
    snes_program_offset = pio_add_program(controller_pio, &snes_controller_output_program);
    start_controller_profile(ControllerProfile::Nes);

    uint8_t received[CHIRBOT_LINK_FRAME_SIZE] = {};
    uint8_t response[CHIRBOT_LINK_FRAME_SIZE] = {};
    uint32_t last_sequence = 0;
    bool have_sequence = false;

    std::printf("NES/SNES output module ready\n");

    while (true) {
        spi_write_read_blocking(kCoreSpi, response, received, CHIRBOT_LINK_FRAME_SIZE);

        chirbot_link_frame_view_t frame;
        if (chirbot_link_decode(received, &frame) != CHIRBOT_LINK_OK ||
            frame.type != CHIRBOT_LINK_FRAME_TASD ||
            (have_sequence && frame.sequence == last_sequence)) {
            continue;
        }

        chirbot_controller_state_t state;
        if (chirbot_controller_tasd_decode(frame.payload, frame.payload_length,
                                           &state) != CHIRBOT_CONTROLLER_TASD_OK ||
            state.port != 0u) {
            continue;
        }

        const ControllerProfile profile = state.inputs_length == 1u
                                              ? ControllerProfile::Nes
                                              : ControllerProfile::Snes;
        const uint32_t pressed_state = state.inputs[0] |
                                       ((uint32_t)state.inputs[1] << 8u);

        wire_state = ~pressed_state;
        if (profile != active_profile) {
            start_controller_profile(profile);
        } else {
            pio_sm_clear_fifos(controller_pio, controller_sm);
            pio_sm_put_blocking(controller_pio, controller_sm, wire_state);
        }

        last_sequence = frame.sequence;
        have_sequence = true;
    }
}