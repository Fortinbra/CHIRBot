#include "output_module_config.hpp"
#include "controller_output_state.hpp"

#include <cinttypes>
#include <cstdio>

#include "chirbot/controller_tasd.h"
#include "chirbot/link_protocol.h"
#include "chirbot/link_spi.h"
#include "chirbot/spi_subnode.h"
#include "controller_output.pio.h"
#include "controller_playback.pio.h"
#include "controller_latch_filter.pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "tasd.h"

namespace {

using chirbot::output::ControllerProfile;

PIO controller_pio = pio0;
uint controller_sm = 0;
uint controller_program_offset = 0;
ControllerProfile active_profile = ControllerProfile::Nes;
uint32_t wire_state = chirbot::output::shift_word(ControllerProfile::Nes, 0);
bool playback_mode = false;
const pio_program_t *controller_program = &controller_output_program;
uint latch_filter_sm = 0;
uint latch_filter_program_offset = 0;
uint16_t latch_filter_us = 0;
uint32_t playback_reports_enqueued = 0;

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

void start_controller_profile(ControllerProfile profile, bool playback = false)
{
    pio_sm_set_enabled(controller_pio, controller_sm, false);
    pio_sm_set_enabled(controller_pio, latch_filter_sm, false);
    pio_interrupt_clear(controller_pio, 0);
    pio_interrupt_clear(controller_pio, 1);
    pio_sm_clear_fifos(controller_pio, controller_sm);
    pio_sm_restart(controller_pio, controller_sm);
    const pio_program_t *const program = playback
        ? &controller_playback_program : &controller_output_program;
    if (program != controller_program) {
        pio_remove_program(controller_pio, controller_program, controller_program_offset);
        controller_program_offset = pio_add_program(controller_pio, program);
        controller_program = program;
    }

    pio_sm_config config = playback
        ? controller_playback_program_get_default_config(controller_program_offset)
        : controller_output_program_get_default_config(controller_program_offset);

    sm_config_set_out_pins(&config, kConsoleDataPin, 1);
    sm_config_set_set_pins(&config, kConsoleDataPin, 1);
    sm_config_set_out_shift(&config, true, false, 32);
    sm_config_set_in_pins(&config, kConsoleClockPin);
    sm_config_set_in_shift(&config, false, false, 32);
    sm_config_set_jmp_pin(&config, kConsoleLatchPin);
    sm_config_set_mov_status(&config, playback && latch_filter_us != 0u
                                        ? STATUS_IRQ_SET : STATUS_TX_LESSTHAN, 0);

    if (playback && latch_filter_us != 0u) {
        pio_sm_config timer_config =
            controller_latch_filter_program_get_default_config(latch_filter_program_offset);
        sm_config_set_clkdiv(&timer_config,
                            static_cast<float>(clock_get_hz(clk_sys)) / 1'000'000.0f);
        pio_sm_init(controller_pio, latch_filter_sm, latch_filter_program_offset, &timer_config);
        pio_sm_put_blocking(controller_pio, latch_filter_sm, latch_filter_us - 1u);
        pio_sm_exec(controller_pio, latch_filter_sm, pio_encode_pull(false, true));
        pio_sm_exec(controller_pio, latch_filter_sm, pio_encode_mov(pio_x, pio_osr));
        pio_sm_set_enabled(controller_pio, latch_filter_sm, true);
    }

    pio_sm_init(controller_pio, controller_sm, controller_program_offset, &config);
    pio_sm_set_pins_with_mask(controller_pio, controller_sm,
                              profile == ControllerProfile::Snes
                                  ? 0u : 1u << kConsoleDataPin,
                              1u << kConsoleDataPin);
    gpio_set_outover(kConsoleDataPin, profile == ControllerProfile::Snes
                                        ? GPIO_OVERRIDE_INVERT
                                        : GPIO_OVERRIDE_NORMAL);
    pio_gpio_init(controller_pio, kConsoleDataPin);
    pio_sm_set_consecutive_pindirs(controller_pio, controller_sm,
                                   kConsoleDataPin, 1, true);
    pio_sm_put_blocking(controller_pio, controller_sm, wire_state);
    pio_sm_set_enabled(controller_pio, controller_sm, true);
    // The startup seed is not a queued playback report.
    while (!pio_sm_is_tx_fifo_empty(controller_pio, controller_sm)) {
        tight_loop_contents();
    }
    active_profile = profile;
    playback_mode = playback;
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
    gpio_init(kConsoleOutputEnablePin);
    gpio_set_dir(kConsoleOutputEnablePin, GPIO_IN);
    gpio_pull_down(kConsoleOutputEnablePin);

    gpio_init(kConsoleLatchPin);
    gpio_set_dir(kConsoleLatchPin, GPIO_IN);
    gpio_disable_pulls(kConsoleLatchPin);
    gpio_init(kConsoleClockPin);
    gpio_set_dir(kConsoleClockPin, GPIO_IN);
    gpio_disable_pulls(kConsoleClockPin);

    controller_sm = pio_claim_unused_sm(controller_pio, true);
    latch_filter_sm = pio_claim_unused_sm(controller_pio, true);
    latch_filter_program_offset = pio_add_program(controller_pio, &controller_latch_filter_program);
    controller_program_offset = pio_add_program(controller_pio, &controller_output_program);
    start_controller_profile(ControllerProfile::Nes);
    init_core_spi();
    stdio_init_all();

    uint8_t received[CHIRBOT_LINK_FRAME_SIZE] = {};
    uint8_t response[CHIRBOT_LINK_FRAME_SIZE] = {};
    uint32_t last_sequence = 0;
    uint8_t last_frame_type = CHIRBOT_LINK_FRAME_EMPTY;
    bool have_sequence = false;
    uint32_t transfers = 0;
    uint32_t cs_timeouts = 0;
    uint32_t decode_errors = 0;
    uint32_t state_errors = 0;
    uint32_t other_port_frames = 0;
    absolute_time_t next_state_error_report = get_absolute_time();
    absolute_time_t next_report = make_timeout_time_ms(2000);

    std::printf("NES/SNES output module ready: live port=%u playback port=%u BSS138 GP%u unused\n",
                chirbot::output::kLiveControllerPort,
                chirbot::output::kPlaybackControllerPort, kConsoleOutputEnablePin);

    while (true) {
        const uint8_t status[] = {
            static_cast<uint8_t>(have_sequence),
            static_cast<uint8_t>(playback_mode
                ? CHIRBOT_LINK_PLAYBACK_QUEUE_CAPACITY -
                    pio_sm_get_tx_fifo_level(controller_pio, controller_sm)
                : CHIRBOT_LINK_PLAYBACK_QUEUE_CAPACITY)
        };
        chirbot_link_encode(response, CHIRBOT_LINK_FRAME_OUTPUT_STATUS,
                            last_sequence, status, sizeof(status));
        if (!chirbot_spi_subnode_transfer(kCoreSpi, kCoreChipSelectPin, response,
                                          received, CHIRBOT_LINK_FRAME_SIZE)) {
            ++cs_timeouts;
        }
        ++transfers;

        if (absolute_time_diff_us(get_absolute_time(), next_report) <= 0) {
            std::printf("[out] transfers=%" PRIu32 " cs_timeouts=%" PRIu32
                        " decode_errors=%" PRIu32 " state_errors=%" PRIu32
                        " other_ports=%" PRIu32 " playback_reports=%" PRIu32 "\n",
                        transfers, cs_timeouts, decode_errors, state_errors,
                        other_port_frames, playback_reports_enqueued);
            next_report = make_timeout_time_ms(2000);
        }

        chirbot_link_frame_view_t frame;
        if (chirbot_link_decode(received, &frame) != CHIRBOT_LINK_OK) {
            ++decode_errors;
            continue;
        }
        if (have_sequence && frame.sequence == last_sequence && frame.type == last_frame_type) {
            continue;
        }
        if (frame.type == CHIRBOT_LINK_FRAME_PLAYBACK_CONFIG) {
            if (frame.payload_length != 2u) {
                ++state_errors;
                std::printf("[out] rejected playback filter configuration\n");
                continue;
            }
            latch_filter_us = static_cast<uint16_t>(
                (static_cast<uint16_t>(frame.payload[0]) << 8u) | frame.payload[1]);
            std::printf("[out] playback latch_filter_us=%u (next begin)\n", latch_filter_us);
            last_sequence = frame.sequence;
            last_frame_type = frame.type;
            have_sequence = true;
            continue;
        }
        if (frame.type == CHIRBOT_LINK_FRAME_PLAYBACK_STOP && frame.payload_length == 0u) {
            wire_state = chirbot::output::shift_word(active_profile, 0);
            start_controller_profile(active_profile);
            last_sequence = frame.sequence;
            last_frame_type = frame.type;
            have_sequence = true;
            continue;
        }
        if (frame.type != CHIRBOT_LINK_FRAME_TASD &&
            frame.type != CHIRBOT_LINK_FRAME_PLAYBACK_BEGIN &&
            frame.type != CHIRBOT_LINK_FRAME_PLAYBACK_STATE) {
            continue;
        }

        chirbot_controller_state_t state = {};
        const chirbot_controller_tasd_result_t state_result =
            chirbot_controller_tasd_decode(frame.payload, frame.payload_length, &state);
        if (state_result != CHIRBOT_CONTROLLER_TASD_OK) {
            ++state_errors;
            if (absolute_time_diff_us(get_absolute_time(), next_state_error_report) <= 0) {
                std::printf("[out] rejected state seq=%" PRIu32
                            " result=%d port=%u type=0x%04x inputs=%u payload=%u hex=",
                            frame.sequence, static_cast<int>(state_result),
                            state.port, state.controller_type, state.inputs_length,
                            frame.payload_length);
                for (uint16_t index = 0; index < frame.payload_length; ++index) {
                    std::printf("%02x", frame.payload[index]);
                }
                std::printf("\n");
                next_state_error_report = make_timeout_time_ms(2000);
            }
            continue;
        }
        if (!chirbot::output::accepts_controller_port(frame.type, state.port)) {
            ++other_port_frames;
            if (frame.type == CHIRBOT_LINK_FRAME_TASD) {
                last_sequence = frame.sequence;
                last_frame_type = frame.type;
                have_sequence = true;
            }
            continue;
        }

        const ControllerProfile profile = state.controller_type == TASD_CTRL_NES_STANDARD
                                              ? ControllerProfile::Nes
                                              : ControllerProfile::Snes;
        const uint16_t pressed_state = static_cast<uint16_t>(
            state.inputs[0] | (static_cast<uint16_t>(state.inputs[1]) << 8u));

        const uint32_t next_wire_state = chirbot::output::shift_word(profile, pressed_state);
        if (frame.type == CHIRBOT_LINK_FRAME_PLAYBACK_BEGIN) {
            playback_reports_enqueued = 0;
            wire_state = chirbot::output::shift_word(profile, 0);
            start_controller_profile(profile, true);
        } else if (frame.type == CHIRBOT_LINK_FRAME_PLAYBACK_STATE) {
            if (!playback_mode || profile != active_profile) {
                ++state_errors;
                if (absolute_time_diff_us(get_absolute_time(), next_state_error_report) <= 0) {
                    std::printf("[out] rejected playback mode/profile seq=%" PRIu32 "\n",
                                frame.sequence);
                    next_state_error_report = make_timeout_time_ms(2000);
                }
                continue;
            }
            if (pio_sm_is_tx_fifo_full(controller_pio, controller_sm)) {
                continue;
            }
            pio_sm_put(controller_pio, controller_sm, next_wire_state);
            ++playback_reports_enqueued;
        } else {
            wire_state = next_wire_state;
            if (profile != active_profile || playback_mode) {
                start_controller_profile(profile);
            } else {
                pio_sm_clear_fifos(controller_pio, controller_sm);
                pio_sm_put_blocking(controller_pio, controller_sm, wire_state);
            }
        }

        last_sequence = frame.sequence;
        last_frame_type = frame.type;
        have_sequence = true;
        if (frame.type == CHIRBOT_LINK_FRAME_TASD) {
            print_button_state(frame.sequence, state);
        }
    }
}