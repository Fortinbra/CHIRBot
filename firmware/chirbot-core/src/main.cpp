#include "chirbot_core_config.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "chirbot/link_protocol.h"
#include "chirbot/link_spi.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "splash_screen.hpp"
#include "st7735_display.hpp"
#include "menu.hpp"
#include "sd_probe.hpp"
#include "sd_storage.hpp"
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

// Updates the on-device display for an input-moment packet. No UART output;
// per-packet printing was too noisy for continuous live relay traffic.
void update_display_from_event(const tasd_packet_t &packet,
                               chirbot::display::St7735Display &display)
{
    if (packet.key != TASD_KEY_INPUT_MOMENT) {
        return;
    }
    tasd_pkt_input_moment_t event;
    if (tasd_decode_input_moment(&packet, &event) == TASD_OK && event.inputs_len == 1u) {
        chirbot::display::draw_nes_button_state(display, event.inputs[0]);
    }
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
        update_display_from_event(packet, display);
    }
    if (result != TASD_ERR_END) {
        std::printf("[TASD seq=%" PRIu32 "] parse error: %d\r\n", sequence, result);
        return false;
    }
    return true;
}

}  // namespace

void run_controller_input(chirbot::display::St7735Display &display)
{
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

    std::printf("CHIRBot core ready: SPI %u Hz, UART stdio\r\n", kModuleSpiBaud);

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

int main()
{
    stdio_init_all();

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

    chirbot::menu::init_buttons();
    const chirbot::menu::Mode mode = chirbot::menu::run_startup_menu(display);

    switch (mode) {
    case chirbot::menu::Mode::FilePlayback:
        chirbot::menu::run_file_playback(display);
        break;
    case chirbot::menu::Mode::SdTools:
        chirbot::menu::run_sd_tools(display);
        break;
    default:
        run_controller_input(display);
        break;
    }
}