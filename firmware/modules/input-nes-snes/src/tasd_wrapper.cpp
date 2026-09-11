#include "nes_input_module.hpp"
#include "chirbot/link_protocol.h"
#include "pico/time.h"
#include <tasd.h>

void NESInputModule::encode_tasd_packet(uint16_t controller_data) {
    uint8_t tasd_document[CHIRBOT_LINK_PAYLOAD_SIZE];
    tasd_writer_t writer;
    tasd_writer_init(&writer, tasd_document, sizeof(tasd_document));

    uint8_t controller_payload[] = {
        0,
        (uint8_t)(TASD_CTRL_NES_STANDARD >> 8u),
        (uint8_t)TASD_CTRL_NES_STANDARD
    };

    const uint8_t input = (uint8_t)controller_data;
    const tasd_pkt_input_moment_t event = {
        .port = 0,
        .hold = 0,
        .index_type = TASD_INDEX_MILLISECONDS,
        .index = time_us_64() / 1000u,
        .inputs = &input,
        .inputs_len = 1
    };

    uint8_t input_payload[12];
    const int input_length = tasd_encode_input_moment(
        &event, input_payload, sizeof(input_payload));

    tasd_result_t result = tasd_writer_write_header(&writer);
    if (result == TASD_OK) {
        result = tasd_writer_append(&writer, TASD_KEY_PORT_CONTROLLER,
                                    controller_payload, sizeof(controller_payload));
    }
    if (result == TASD_OK && input_length > 0) {
        result = tasd_writer_append(&writer, TASD_KEY_INPUT_MOMENT,
                                    input_payload, (uint32_t)input_length);
    }
    if (result == TASD_OK) {
        chirbot_link_encode(tx_frame_, CHIRBOT_LINK_FRAME_TASD, ++sequence_,
                            tasd_document, (uint16_t)tasd_writer_size(&writer));
    }
}
