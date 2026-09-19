#include "chirbot_core_config.hpp"

#include <cinttypes>
#include <cstdio>

#include "chirbot/link_protocol.h"
#include "chirbot/link_spi.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "splash_screen.hpp"
#include "st7735_display.hpp"
#include "tasd.h"

namespace {

void init_spi_main(spi_inst_t *spi, uint miso, uint chip_select, uint clock, uint mosi)
{
    spi_init(spi, kModuleSpiBaud);
    spi_set_format(spi, CHIRBOT_LINK_SPI_DATA_BITS, CHIRBOT_LINK_SPI_CPOL,
                   CHIRBOT_LINK_SPI_CPHA, CHIRBOT_LINK_SPI_ORDER);

    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(clock, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_init(chip_select);
    gpio_set_dir(chip_select, GPIO_OUT);
    gpio_put(chip_select, 1);
}

void transfer_frame(spi_inst_t *spi, uint chip_select, const uint8_t *tx, uint8_t *rx)
{
    gpio_put(chip_select, 0);
    spi_write_read_blocking(spi, tx, rx, CHIRBOT_LINK_FRAME_SIZE);
    gpio_put(chip_select, 1);
}

void forward_frame(const uint8_t *frame)
{
    gpio_put(kOutputChipSelectPin, 0);
    spi_write_blocking(kOutputSpi, frame, CHIRBOT_LINK_FRAME_SIZE);
    gpio_put(kOutputChipSelectPin, 1);
}

void print_bytes(const uint8_t *bytes, uint32_t length)
{
    for (uint32_t index = 0; index < length; ++index) {
        std::printf("%02x", bytes[index]);
    }
}

void print_nes_buttons(uint8_t state)
{
    static const char *const names[] = {
        "A", "B", "Select", "Start", "Up", "Down", "Left", "Right"
    };

    bool first = true;
    for (uint8_t bit = 0; bit < 8u; ++bit) {
        if ((state & (1u << bit)) != 0u) {
            std::printf("%s%s", first ? "" : ",", names[bit]);
            first = false;
        }
    }
    if (first) {
        std::printf("none");
    }
}

const char *nes_button_label(uint8_t state)
{
    static const char *const labels[] = {
        "A", "B", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT"
    };

    for (uint8_t bit = 0; bit < 8u; ++bit) {
        if ((state & (1u << bit)) != 0u) {
            return labels[bit];
        }
    }
    return "----";
}

// Demo rendering: shows the first pressed button, centered, one at a time.
void show_buttons(chirbot::display::St7735Display &display, uint8_t state)
{
    using chirbot::display::St7735Display;

    const char *label = nes_button_label(state);
    int16_t length = 0;
    while (label[length] != '\0') {
        ++length;
    }

    constexpr uint8_t kScale = 2;
    constexpr int16_t kGlyphWidth = 6 * kScale;
    const int16_t text_width = static_cast<int16_t>(length * kGlyphWidth - kScale);
    const int16_t x = static_cast<int16_t>((St7735Display::kWidth - text_width) / 2);
    const int16_t y = static_cast<int16_t>((St7735Display::kHeight - 7 * kScale) / 2);

    display.fill_screen(chirbot::display::rgb565(0, 0, 0));
    display.draw_text(x, y, label, chirbot::display::rgb565(255, 255, 255), kScale);
}

void print_tasd_event(uint32_t sequence, const tasd_packet_t &packet,
                      chirbot::display::St7735Display &display)
{
    std::printf("[TASD seq=%" PRIu32 "] key=0x%04x len=%" PRIu32,
                sequence, packet.key, packet.payload_len);

    if (packet.key == TASD_KEY_INPUT_MOMENT) {
        tasd_pkt_input_moment_t event;
        if (tasd_decode_input_moment(&packet, &event) == TASD_OK) {
            std::printf(" port=%u hold=%u index_type=%u index=%" PRIu64 " data=",
                        event.port, event.hold, event.index_type, event.index);
            print_bytes(event.inputs, event.inputs_len);
            if (event.inputs_len == 1u) {
                std::printf(" buttons=");
                print_nes_buttons(event.inputs[0]);
                show_buttons(display, event.inputs[0]);
            }
        } else {
            std::printf(" invalid_input_moment");
        }
    } else if (packet.key == TASD_KEY_PORT_CONTROLLER) {
        tasd_pkt_port_controller_t controller;
        if (tasd_decode_port_controller(&packet, &controller) == TASD_OK) {
            std::printf(" port=%u controller=0x%04x", controller.port, controller.type);
        } else {
            std::printf(" invalid_port_controller");
        }
    } else {
        std::printf(" payload=");
        print_bytes(packet.payload, packet.payload_len);
    }

    std::printf("\r\n");
}

bool inspect_tasd_document(uint32_t sequence, const uint8_t *payload, uint16_t length,
                           chirbot::display::St7735Display &display)
{
    tasd_header_t header;
    const tasd_result_t header_result = tasd_read_header(payload, length, &header);
    if (header_result != TASD_OK) {
        std::printf("[TASD seq=%" PRIu32 "] invalid header: %d\r\n",
                    sequence, header_result);
        return false;
    }
    if (header.version != TASD_VERSION || header.g_keylen != TASD_G_KEYLEN) {
        std::printf("[TASD seq=%" PRIu32
                    "] unsupported header: version=%u keylen=%u\r\n",
                    sequence, header.version, header.g_keylen);
        return false;
    }

    tasd_reader_t reader;
    tasd_reader_init(&reader, payload, length, &header);

    tasd_packet_t packet;
    tasd_result_t result;
    while ((result = tasd_reader_next(&reader, &packet)) == TASD_OK) {
        print_tasd_event(sequence, packet, display);
    }
    if (result != TASD_ERR_END) {
        std::printf("[TASD seq=%" PRIu32 "] parse error: %d\r\n", sequence, result);
        return false;
    }
    return true;
}

}  // namespace

int main()
{
    stdio_init_all();
    // Give USB CDC time to enumerate so startup logging is not lost.
    sleep_ms(2000);

    chirbot::display::St7735Display display({
        .clock = kDisplayClockPin,
        .mosi = kDisplayMosiPin,
        .chip_select = kDisplayChipSelectPin,
        .data_command = kDisplayDataCommandPin,
        .reset = kDisplayResetPin,
        .backlight = kDisplayBacklightPin,
    });
    display.init();
    chirbot::display::show_rainbow_splash(display, "CHIRbot", 2000);
    display.fill_screen(chirbot::display::rgb565(0, 0, 0));

    init_spi_main(kInputSpi, kInputMisoPin, kInputChipSelectPin,
                  kInputClockPin, kInputMosiPin);
    init_spi_main(kOutputSpi, kOutputMisoPin, kOutputChipSelectPin,
                  kOutputClockPin, kOutputMosiPin);

    uint8_t request[CHIRBOT_LINK_FRAME_SIZE];
    uint8_t received[CHIRBOT_LINK_FRAME_SIZE];
    chirbot_link_encode(request, CHIRBOT_LINK_FRAME_EMPTY, 0, nullptr, 0);

    bool have_sequence = false;
    uint32_t last_sequence = 0;
    chirbot_link_result_t last_error = CHIRBOT_LINK_OK;
    uint32_t polls = 0;
    uint32_t good_frames = 0;
    uint32_t tasd_frames = 0;
    absolute_time_t next_report = make_timeout_time_ms(2000);

    std::printf("CHIRBot core ready: SPI %u Hz, USB stdio\r\n", kModuleSpiBaud);

    while (true) {
        transfer_frame(kInputSpi, kInputChipSelectPin, request, received);
        ++polls;

        chirbot_link_frame_view_t frame;
        const chirbot_link_result_t result = chirbot_link_decode(received, &frame);
        if (result == CHIRBOT_LINK_OK) {
            ++good_frames;
            if (last_error != CHIRBOT_LINK_OK) {
                std::printf("Input link recovered\r\n");
            }
            last_error = CHIRBOT_LINK_OK;

            if (frame.type == CHIRBOT_LINK_FRAME_TASD &&
                (!have_sequence || frame.sequence != last_sequence)) {
                ++tasd_frames;
                if (inspect_tasd_document(frame.sequence, frame.payload,
                                          frame.payload_length, display)) {
                    forward_frame(received);
                }
                last_sequence = frame.sequence;
                have_sequence = true;
            }
        } else if (result != last_error) {
            std::printf("Input link error: %d\r\n", result);
            last_error = result;
        }

        if (absolute_time_diff_us(get_absolute_time(), next_report) <= 0) {
            std::printf("[core] polls=%" PRIu32 " ok=%" PRIu32 " tasd=%" PRIu32
                        " last_err=%d rx=%02x%02x%02x%02x%02x%02x\r\n",
                        polls, good_frames, tasd_frames, last_error,
                        received[0], received[1], received[2],
                        received[3], received[4], received[5]);
            next_report = make_timeout_time_ms(2000);
        }

        sleep_us(kInputPollIntervalUs);
    }
}