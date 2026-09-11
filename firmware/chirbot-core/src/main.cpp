#include "chirbot_core_config.hpp"

#include <cinttypes>
#include <cstdio>

#include "chirbot/link_protocol.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "tasd.h"

namespace {

void init_spi_main(spi_inst_t *spi, uint miso, uint chip_select, uint clock, uint mosi)
{
    spi_init(spi, kModuleSpiBaud);
    spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

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

void print_tasd_event(uint32_t sequence, const tasd_packet_t &packet)
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

void print_tasd_document(uint32_t sequence, const uint8_t *payload, uint16_t length)
{
    tasd_header_t header;
    const tasd_result_t header_result = tasd_read_header(payload, length, &header);
    if (header_result != TASD_OK) {
        std::printf("[TASD seq=%" PRIu32 "] invalid header: %d\r\n",
                    sequence, header_result);
        return;
    }

    tasd_reader_t reader;
    tasd_reader_init(&reader, payload, length, &header);

    tasd_packet_t packet;
    tasd_result_t result;
    while ((result = tasd_reader_next(&reader, &packet)) == TASD_OK) {
        print_tasd_event(sequence, packet);
    }
    if (result != TASD_ERR_END) {
        std::printf("[TASD seq=%" PRIu32 "] parse error: %d\r\n", sequence, result);
    }
}

}  // namespace

int main()
{
    stdio_init_all();
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

    std::printf("CHIRBot core ready: SPI %u Hz, UART stdio\r\n", kModuleSpiBaud);

    while (true) {
        transfer_frame(kInputSpi, kInputChipSelectPin, request, received);

        chirbot_link_frame_view_t frame;
        const chirbot_link_result_t result = chirbot_link_decode(received, &frame);
        if (result == CHIRBOT_LINK_OK) {
            if (last_error != CHIRBOT_LINK_OK) {
                std::printf("Input link recovered\r\n");
            }
            last_error = CHIRBOT_LINK_OK;

            if (frame.type == CHIRBOT_LINK_FRAME_TASD &&
                (!have_sequence || frame.sequence != last_sequence)) {
                print_tasd_document(frame.sequence, frame.payload, frame.payload_length);
                forward_frame(received);
                last_sequence = frame.sequence;
                have_sequence = true;
            }
        } else if (result != last_error) {
            std::printf("Input link error: %d\r\n", result);
            last_error = result;
        }

        sleep_us(kInputPollIntervalUs);
    }
}