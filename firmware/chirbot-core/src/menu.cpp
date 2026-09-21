#include "menu.hpp"

#include "chirbot_core_config.hpp"
#include "sd_storage.hpp"

#include "chirbot/controller_tasd.h"
#include "chirbot/link_protocol.h"
#include "chirbot/link_spi.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "tasd.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace chirbot::menu {
namespace {

using chirbot::display::St7735Display;
using chirbot::display::rgb565;

constexpr uint32_t kDebounceUs = 20'000;
constexpr uint32_t kStartupTimeoutMs = 5000;
constexpr uint32_t kFormatConfirmTimeoutMs = 10'000;

struct ButtonState {
    uint pin;
    bool raw_last = false;
    bool stable_pressed = false;
    absolute_time_t change_time{};
};

ButtonState g_buttons[3] = {
    {kMenuUpPin}, {kMenuDownPin}, {kMenuSelectPin}
};

void draw_list(St7735Display &display, const char *title, const char *const *items,
               const bool *disabled, int count, int highlighted, const char *status)
{
    constexpr int16_t kRowHeight = 10;
    constexpr int16_t kFirstRowY = 14;

    display.fill_screen(rgb565(0, 0, 0));
    display.draw_text(2, 2, title, rgb565(255, 255, 255), 1);

    for (int index = 0; index < count; ++index) {
        const int16_t y = static_cast<int16_t>(kFirstRowY + index * kRowHeight);
        if (y > St7735Display::kHeight - kRowHeight) {
            break;
        }
        if (index == highlighted) {
            display.fill_rect(0, static_cast<int16_t>(y - 1), St7735Display::kWidth,
                              kRowHeight, rgb565(0, 0, 140));
        }
        const bool is_disabled = disabled != nullptr && disabled[index];
        display.draw_text(4, y, items[index],
                          is_disabled ? rgb565(100, 100, 100) : rgb565(255, 255, 255), 1);
    }

    if (status != nullptr) {
        display.draw_text(2, static_cast<int16_t>(St7735Display::kHeight - 9), status,
                          rgb565(160, 160, 160), 1);
    }
}

// Moves `index` one step in `direction` (+1/-1), skipping disabled entries.
// Returns `index` unchanged if every other entry is disabled.
int step(int index, int direction, const bool *disabled, int count)
{
    for (int attempt = 0; attempt < count; ++attempt) {
        index = (index + direction + count) % count;
        if (!disabled[index]) {
            return index;
        }
    }
    return index;
}

bool output_spi_ready = false;

// run_controller_input() initializes this SPI link itself; File Playback
// bypasses that path entirely, so it must init the link before sending.
void init_output_spi_once()
{
    if (output_spi_ready) {
        return;
    }
    spi_init(kOutputSpi, kModuleSpiBaud);
    spi_set_format(kOutputSpi, CHIRBOT_LINK_SPI_DATA_BITS, CHIRBOT_LINK_SPI_CPOL,
                   CHIRBOT_LINK_SPI_CPHA, CHIRBOT_LINK_SPI_ORDER);
    gpio_set_function(kOutputMisoPin, GPIO_FUNC_SPI);
    gpio_set_function(kOutputClockPin, GPIO_FUNC_SPI);
    gpio_set_function(kOutputMosiPin, GPIO_FUNC_SPI);
    gpio_init(kOutputChipSelectPin);
    gpio_set_dir(kOutputChipSelectPin, GPIO_OUT);
    gpio_put(kOutputChipSelectPin, 1);
    output_spi_ready = true;
}

void send_output_moment(uint8_t port, uint16_t controller_type, const uint8_t *inputs,
                        uint32_t inputs_len, uint64_t index, uint32_t &sequence)
{
    chirbot_controller_state_t state{};
    state.port = port;
    state.controller_type = controller_type;
    state.inputs_length = static_cast<uint8_t>(
        inputs_len > CHIRBOT_CONTROLLER_MAX_INPUT_SIZE ? CHIRBOT_CONTROLLER_MAX_INPUT_SIZE : inputs_len);
    for (uint8_t i = 0; i < state.inputs_length; ++i) {
        state.inputs[i] = inputs[i];
    }
    state.index = index;

    uint8_t document[32];
    uint16_t document_length = 0;
    if (chirbot_controller_tasd_encode(&state, document, sizeof(document), &document_length) !=
        CHIRBOT_CONTROLLER_TASD_OK) {
        return;
    }

    uint8_t frame[CHIRBOT_LINK_FRAME_SIZE];
    if (chirbot_link_encode(frame, CHIRBOT_LINK_FRAME_TASD, sequence, document, document_length) !=
        CHIRBOT_LINK_OK) {
        return;
    }
    ++sequence;

    gpio_put(kOutputChipSelectPin, 0);
    spi_write_blocking(kOutputSpi, frame, CHIRBOT_LINK_FRAME_SIZE);
    gpio_put(kOutputChipSelectPin, 1);
}

// File-scope storage, not a local: main() runs with a small stack, and this
// buffer is loaded once per selected file, not per call. Files larger than
// this are played back truncated.
constexpr size_t kPlaybackBufferCapacity = 393'216;  // 384 KiB
uint8_t g_playback_buffer[kPlaybackBufferCapacity];

void show_message(St7735Display &display, const char *line1, const char *line2, uint16_t color)
{
    display.fill_screen(rgb565(0, 0, 0));
    display.draw_text(4, 4, line1, color, 1);
    if (line2 != nullptr) {
        display.draw_text(4, 16, line2, color, 1);
    }
}

void play_file(St7735Display &display, const char *name)
{
    size_t length = 0;
    size_t file_size = 0;
    if (!chirbot::sd::read_tasd_file(name, g_playback_buffer, kPlaybackBufferCapacity,
                                     &length, &file_size)) {
        std::printf("Playback: failed to read %s\r\n", name);
        show_message(display, "Failed to read file", name, rgb565(255, 60, 60));
        sleep_ms(1500);
        return;
    }
    std::printf("Playback: read %s, %zu of %zu bytes\r\n", name, length, file_size);

    tasd_header_t header;
    if (tasd_read_header(g_playback_buffer, length, &header) != TASD_OK ||
        header.version != TASD_VERSION || header.g_keylen != TASD_G_KEYLEN) {
        std::printf("Playback: invalid header (version=%u keylen=%u)\r\n",
                    header.version, header.g_keylen);
        show_message(display, "Invalid TASD file", name, rgb565(255, 60, 60));
        sleep_ms(1500);
        return;
    }

    tasd_reader_t reader;
    tasd_reader_init(&reader, g_playback_buffer, length, &header);

    show_message(display, "Playing:", name, rgb565(255, 255, 255));

    uint8_t port = 0;
    uint16_t controller_type = TASD_CTRL_NES_STANDARD;
    uint32_t sequence = 0;
    bool have_start = false;
    uint8_t start_index_type = 0;
    uint64_t first_index = 0;
    uint64_t chunk_frame = 0;
    absolute_time_t start_time{};
    // TASD movies commonly index by frame count (TASD_INDEX_FRAME), not
    // milliseconds; NTSC is the default absent a CONSOLE_REGION packet.
    double frames_per_second = 60.0988;

    // A corrupt or unexpected index value must never hang playback forever;
    // cap any single wait and let SELECT abort at any time.
    constexpr uint32_t kMaxWaitMs = 3000;
    bool aborted = false;
    uint32_t moments_played = 0;
    uint32_t chunk_bytes_played = 0;
    uint32_t other_packets = 0;

    // Real files can carry thousands of tiny INPUT_CHUNK packets; printing
    // one line per packet floods a slow UART and looks like a hang. Report
    // progress on a timer instead.
    absolute_time_t next_progress_report = make_timeout_time_ms(500);
    auto report_progress = [&]() {
        if (absolute_time_diff_us(get_absolute_time(), next_progress_report) > 0) {
            return;
        }
        std::printf("Playback: progress moments=%" PRIu32 " chunk_bytes=%" PRIu32
                    " other=%" PRIu32 "\r\n",
                    moments_played, chunk_bytes_played, other_packets);
        next_progress_report = make_timeout_time_ms(500);
    };

    auto wait_until = [&](absolute_time_t due_time) {
        const absolute_time_t capped = delayed_by_ms(get_absolute_time(), kMaxWaitMs);
        if (absolute_time_diff_us(capped, due_time) > 0) {
            due_time = capped;
        }
        while (absolute_time_diff_us(get_absolute_time(), due_time) > 0) {
            if (poll_button() == Button::Select) {
                aborted = true;
                return;
            }
        }
    };

    tasd_packet_t packet;
    tasd_result_t result;
    while (!aborted && (result = tasd_reader_next(&reader, &packet)) == TASD_OK) {
        if (packet.key == TASD_KEY_CONSOLE_REGION && packet.payload_len >= 1u &&
            packet.payload[0] == TASD_REGION_PAL) {
            frames_per_second = 50.0;
            continue;
        }
        if (packet.key == TASD_KEY_PORT_CONTROLLER) {
            tasd_pkt_port_controller_t controller;
            if (tasd_decode_port_controller(&packet, &controller) == TASD_OK) {
                port = controller.port;
                controller_type = controller.type;
            }
            continue;
        }

        // Real-world TASD movies (e.g. downloaded from tasd.io) typically
        // store the whole recording as one or a few bulk INPUT_CHUNK packets
        // (one raw controller byte per frame) rather than a separate
        // INPUT_MOMENT packet per frame, which is what the live SPI link
        // uses instead. Both are supported here.
        if (packet.key == TASD_KEY_INPUT_CHUNK) {
            tasd_pkt_input_chunk_t chunk;
            if (tasd_decode_input_chunk(&packet, &chunk) != TASD_OK || chunk.inputs_len == 0u) {
                continue;
            }
            for (uint32_t i = 0; i < chunk.inputs_len && !aborted; ++i) {
                if (!have_start) {
                    have_start = true;
                    start_time = get_absolute_time();
                } else {
                    const uint64_t due_ms = static_cast<uint64_t>(
                        static_cast<double>(chunk_frame) * 1000.0 / frames_per_second);
                    wait_until(delayed_by_ms(start_time, due_ms));
                }
                if (aborted) {
                    break;
                }
                send_output_moment(chunk.port, controller_type, &chunk.inputs[i], 1,
                                   chunk_frame, sequence);
                chirbot::display::draw_nes_button_state(display, chunk.inputs[i]);
                ++chunk_frame;
                ++chunk_bytes_played;
                report_progress();
            }
            continue;
        }

        if (packet.key != TASD_KEY_INPUT_MOMENT) {
            ++other_packets;
            report_progress();
            continue;
        }

        tasd_pkt_input_moment_t event;
        if (tasd_decode_input_moment(&packet, &event) != TASD_OK || event.inputs_len == 0u) {
            continue;
        }

        // Pace playback against the recorded index. FRAME and MILLISECONDS
        // both convert to elapsed wall-clock milliseconds since the first
        // moment; any other index type falls back to a fixed per-packet
        // delay so playback is still visible instead of racing through with
        // no pacing at all.
        if (event.index_type == TASD_INDEX_MILLISECONDS || event.index_type == TASD_INDEX_FRAME) {
            if (!have_start) {
                have_start = true;
                start_index_type = event.index_type;
                first_index = event.index;
                start_time = get_absolute_time();
            } else if (event.index_type == start_index_type) {
                const uint64_t elapsed = event.index - first_index;
                const uint64_t due_ms = event.index_type == TASD_INDEX_FRAME
                    ? static_cast<uint64_t>(static_cast<double>(elapsed) * 1000.0 / frames_per_second)
                    : elapsed;
                wait_until(delayed_by_ms(start_time, due_ms));
            }
        } else {
            sleep_ms(16);
        }
        if (aborted) {
            break;
        }

        send_output_moment(port, controller_type, event.inputs, event.inputs_len,
                           event.index, sequence);
        if (event.inputs_len == 1u) {
            chirbot::display::draw_nes_button_state(display, event.inputs[0]);
        }
        ++moments_played;
        report_progress();
    }

    std::printf("Playback: %s (moments=%" PRIu32 " chunk_bytes=%" PRIu32
                " other_packets=%" PRIu32 " result=%d)\r\n",
                aborted ? "aborted" : "finished", moments_played, chunk_bytes_played,
                other_packets, static_cast<int>(result));

    if (!aborted && result != TASD_ERR_END) {
        show_message(display, "Parse error", name, rgb565(255, 60, 60));
        sleep_ms(1500);
    }
}

void run_format_confirm(St7735Display &display)
{
    static const char *const kItems[] = {"Cancel", "Confirm erase"};
    int highlighted = 0;
    int last_drawn_highlighted = -1;
    int last_drawn_seconds = -1;
    const absolute_time_t deadline = make_timeout_time_ms(kFormatConfirmTimeoutMs);
    char status[40];

    while (true) {
        const int64_t remaining_us = absolute_time_diff_us(get_absolute_time(), deadline);
        if (remaining_us <= 0) {
            return;  // silence cancels; never format on timeout
        }
        const int seconds_remaining = static_cast<int>(remaining_us / 1000000 + 1);
        if (highlighted != last_drawn_highlighted || seconds_remaining != last_drawn_seconds) {
            std::snprintf(status, sizeof(status), "ERASE SD? auto-cancel in %ds",
                         seconds_remaining);
            draw_list(display, "Format SD", kItems, nullptr, 2, highlighted, status);
            last_drawn_highlighted = highlighted;
            last_drawn_seconds = seconds_remaining;
        }

        const Button button = poll_button();
        if (button == Button::Up || button == Button::Down) {
            highlighted = 1 - highlighted;
        } else if (button == Button::Select) {
            if (highlighted == 0) {
                return;
            }
            show_message(display, "Formatting...", "Do not remove card", rgb565(255, 255, 255));
            const bool ok = chirbot::sd::format_card();
            show_message(display, ok ? "Format complete" : "Format failed", nullptr,
                        ok ? rgb565(0, 255, 0) : rgb565(255, 60, 60));
            sleep_ms(1500);
            return;
        }
        sleep_ms(15);
    }
}

}  // namespace

void init_buttons()
{
    for (ButtonState &state : g_buttons) {
        gpio_init(state.pin);
        gpio_set_dir(state.pin, GPIO_IN);
        gpio_pull_up(state.pin);
    }
    gpio_init(kSdDetectPin);
    gpio_set_dir(kSdDetectPin, GPIO_IN);
    gpio_pull_up(kSdDetectPin);
}

Button poll_button()
{
    static const Button kButtonFor[3] = {Button::Up, Button::Down, Button::Select};
    Button pressed = Button::None;

    for (size_t i = 0; i < 3; ++i) {
        ButtonState &state = g_buttons[i];
        const bool raw = gpio_get(state.pin) == 0;  // active-low
        if (raw != state.raw_last) {
            state.raw_last = raw;
            state.change_time = get_absolute_time();
        } else if (raw != state.stable_pressed &&
                   absolute_time_diff_us(state.change_time, get_absolute_time()) >=
                       static_cast<int64_t>(kDebounceUs)) {
            state.stable_pressed = raw;
            if (raw) {
                pressed = kButtonFor[i];
            }
        }
    }
    return pressed;
}

bool sd_card_present()
{
    // Flipped from the original "low = present" assumption after bring-up
    // showed the opposite: this board's DET switch reads high with a card
    // seated (a normally-closed-to-GND switch that opens on insertion is a
    // common variant). See docs/specs/sd-storage.md for the raw-level
    // readout shown on the startup menu if this needs re-checking again.
    return gpio_get(kSdDetectPin) != 0;
}

Mode run_startup_menu(St7735Display &display)
{
    static const char *const kItems[] = {"Controller Input", "File Playback", "SD Tools"};
    constexpr int kCount = 3;

    const bool card_present = sd_card_present();
    bool disabled[kCount] = {false, !card_present, !card_present};
    int highlighted = 0;
    int last_drawn_highlighted = -1;
    int last_drawn_seconds = -1;
    absolute_time_t deadline = make_timeout_time_ms(kStartupTimeoutMs);
    char status[48];

    while (true) {
        const int64_t remaining_us = absolute_time_diff_us(get_absolute_time(), deadline);
        if (remaining_us <= 0) {
            return Mode::ControllerInput;
        }
        const int seconds_remaining = static_cast<int>(remaining_us / 1000000 + 1);
        if (highlighted != last_drawn_highlighted || seconds_remaining != last_drawn_seconds) {
            if (card_present) {
                std::snprintf(status, sizeof(status), "Auto-select in %ds", seconds_remaining);
            } else {
                std::snprintf(status, sizeof(status), "No SD card (DET=%d): Controller only",
                             gpio_get(kSdDetectPin));
            }
            draw_list(display, "CHIRBot", kItems, disabled, kCount, highlighted, status);
            last_drawn_highlighted = highlighted;
            last_drawn_seconds = seconds_remaining;
        }

        const Button button = poll_button();
        if (button != Button::None) {
            deadline = make_timeout_time_ms(kStartupTimeoutMs);
        }
        if (button == Button::Up) {
            highlighted = step(highlighted, -1, disabled, kCount);
        } else if (button == Button::Down) {
            highlighted = step(highlighted, +1, disabled, kCount);
        } else if (button == Button::Select && !disabled[highlighted]) {
            switch (highlighted) {
            case 0: return Mode::ControllerInput;
            case 1: return Mode::FilePlayback;
            default: return Mode::SdTools;
            }
        }
        sleep_ms(15);
    }
}

void run_file_playback(St7735Display &display)
{
    init_output_spi_once();
    chirbot::sd::mount_filesystem();

    while (true) {
        chirbot::sd::TasdFileEntry entries[12];
        const size_t count = chirbot::sd::list_tasd_files(entries, 12);

        if (count == 0u) {
            show_message(display, "No .tasd files found", "Reset to exit", rgb565(255, 60, 60));
            while (true) {
                sleep_ms(200);
            }
        }

        const char *names[12];
        for (size_t i = 0; i < count; ++i) {
            names[i] = entries[i].name;
        }

        int highlighted = 0;
        constexpr int kVisibleRows = 5;
        int scroll_offset = 0;
        int last_drawn_highlighted = -1;

        while (true) {
            if (highlighted < scroll_offset) {
                scroll_offset = highlighted;
            } else if (highlighted >= scroll_offset + kVisibleRows) {
                scroll_offset = highlighted - kVisibleRows + 1;
            }
            if (highlighted != last_drawn_highlighted) {
                const int visible_count =
                    static_cast<int>(count) - scroll_offset < kVisibleRows
                        ? static_cast<int>(count) - scroll_offset
                        : kVisibleRows;
                draw_list(display, "Select file", names + scroll_offset, nullptr, visible_count,
                         highlighted - scroll_offset, "SELECT to play");
                last_drawn_highlighted = highlighted;
            }

            const Button button = poll_button();
            if (button == Button::Up && highlighted > 0) {
                --highlighted;
            } else if (button == Button::Down &&
                      highlighted + 1 < static_cast<int>(count)) {
                ++highlighted;
            } else if (button == Button::Select) {
                play_file(display, entries[highlighted].name);
                break;  // re-list in case the card contents changed
            }
            sleep_ms(15);
        }
    }
}

void run_sd_tools(St7735Display &display)
{
    // Single item for now: Mass Storage is not implemented.
    static const char *const kItems[] = {"Format SD"};
    int last_drawn = -1;

    while (true) {
        if (last_drawn != 0) {
            draw_list(display, "SD Tools", kItems, nullptr, 1, 0, nullptr);
            last_drawn = 0;
        }

        if (poll_button() == Button::Select) {
            run_format_confirm(display);
            last_drawn = -1;  // force a redraw; the confirm screen overwrote it
        }
        sleep_ms(15);
    }
}

}  // namespace chirbot::menu
