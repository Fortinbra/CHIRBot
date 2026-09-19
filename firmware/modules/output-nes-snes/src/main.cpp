#include "output_module_config.hpp"

#include <cinttypes>
#include <cstdio>

#include "chirbot/controller_tasd.h"
#include "chirbot/link_protocol.h"
#include "chirbot/link_spi.h"
#include "chirbot/spi_subnode.h"
#include "controller_output.pio.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "tasd.h"

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
    spi_init(kCoreSpi, CHIRBOT_LINK_SPI_BAUD);
    spi_set_format(kCoreSpi, CHIRBOT_LINK_SPI_DATA_BITS, CHIRBOT_LINK_SPI_CPOL,
                   CHIRBOT_LINK_SPI_CPHA, CHIRBOT_LINK_SPI_ORDER);
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

void print_button_state(uint32_t sequence, const chirbot_controller_state_t &state)
{
    static const char *const kNesNames[] = {
        "A", "B", "Select", "Start", "Up", "Down", "Left", "Right"
    };

    std::printf("[OUT seq=%" PRIu32 " port=%u type=0x%04x index=%" PRIu64 "] ",
                sequence, state.port, state.controller_type, state.index);

    if (state.controller_type != TASD_CTRL_NES_STANDARD) {
        // Only the NES bit layout is named here; anything else prints raw.
        std::printf("raw=");
        for (uint8_t index = 0; index < state.inputs_length; ++index) {
            std::printf("%02x", state.inputs[index]);
        }
        std::printf("\n");
        return;
    }

    bool first = true;
    for (uint8_t bit = 0; bit < 8u; ++bit) {
        if ((state.inputs[0] & (1u << bit)) != 0u) {
            std::printf("%s%s", first ? "" : ",", kNesNames[bit]);
            first = false;
        }
    }
    if (first) {
        std::printf("none");
    }
    std::printf("\n");
}

}  // namespace

int main()
{
    stdio_init_all();
    // Give USB CDC time to enumerate so startup logging is not lost.
    sleep_ms(2000);
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
    uint32_t transfers = 0;
    uint32_t cs_timeouts = 0;
    uint32_t decode_errors = 0;
    absolute_time_t next_report = make_timeout_time_ms(2000);

    std::printf("NES/SNES output module ready\n");

    while (true) {
        if (!chirbot_spi_subnode_transfer(kCoreSpi, kCoreChipSelectPin, response,
                                          received, CHIRBOT_LINK_FRAME_SIZE)) {
            ++cs_timeouts;
        }
        ++transfers;

        if (absolute_time_diff_us(get_absolute_time(), next_report) <= 0) {
            std::printf("[out] transfers=%" PRIu32 " cs_timeouts=%" PRIu32
                        " decode_errors=%" PRIu32 "\n",
                        transfers, cs_timeouts, decode_errors);
            next_report = make_timeout_time_ms(2000);
        }

        chirbot_link_frame_view_t frame;
        if (chirbot_link_decode(received, &frame) != CHIRBOT_LINK_OK) {
            ++decode_errors;
            continue;
        }
        if (frame.type != CHIRBOT_LINK_FRAME_TASD ||
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

        print_button_state(frame.sequence, state);

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