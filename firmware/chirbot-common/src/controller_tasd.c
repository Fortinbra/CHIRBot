#include "chirbot/controller_tasd.h"

#include <string.h>

#include "tasd.h"

static uint32_t supported_input_size(uint16_t controller_type)
{
    if (controller_type != TASD_CTRL_NES_STANDARD &&
        controller_type != TASD_CTRL_SNES_STANDARD) {
        return 0u;
    }
    return tasd_controller_input_size(controller_type);
}

chirbot_controller_tasd_result_t chirbot_controller_tasd_encode(
    const chirbot_controller_state_t *state,
    uint8_t *document,
    size_t capacity,
    uint16_t *document_length)
{
    uint8_t controller_payload[3];
    uint8_t input_payload[13];
    tasd_pkt_input_moment_t input;
    tasd_writer_t writer;
    tasd_result_t result;
    int payload_length;
    uint32_t expected_input_size;

    if (state == NULL || document == NULL || document_length == NULL) {
        return CHIRBOT_CONTROLLER_TASD_ERR_ARGUMENT;
    }

    expected_input_size = supported_input_size(state->controller_type);
    if (expected_input_size == 0u || state->inputs_length != expected_input_size) {
        return CHIRBOT_CONTROLLER_TASD_ERR_UNSUPPORTED;
    }

    payload_length = tasd_encode_port_controller(
        state->port, state->controller_type,
        controller_payload, sizeof(controller_payload));
    if (payload_length < 0) {
        return CHIRBOT_CONTROLLER_TASD_ERR_BUFFER_SMALL;
    }

    input.port = state->port;
    input.hold = 0u;
    input.index_type = TASD_INDEX_MILLISECONDS;
    input.index = state->index;
    input.inputs = state->inputs;
    input.inputs_len = state->inputs_length;
    payload_length = tasd_encode_input_moment(
        &input, input_payload, sizeof(input_payload));
    if (payload_length < 0) {
        return CHIRBOT_CONTROLLER_TASD_ERR_BUFFER_SMALL;
    }

    tasd_writer_init(&writer, document, capacity);
    result = tasd_writer_write_header(&writer);
    if (result == TASD_OK) {
        result = tasd_writer_append(&writer, TASD_KEY_PORT_CONTROLLER,
                                    controller_payload, sizeof(controller_payload));
    }
    if (result == TASD_OK) {
        result = tasd_writer_append(&writer, TASD_KEY_INPUT_MOMENT,
                                    input_payload, (uint32_t)payload_length);
    }
    if (result != TASD_OK || tasd_writer_size(&writer) > UINT16_MAX) {
        return CHIRBOT_CONTROLLER_TASD_ERR_BUFFER_SMALL;
    }

    *document_length = (uint16_t)tasd_writer_size(&writer);
    return CHIRBOT_CONTROLLER_TASD_OK;
}

chirbot_controller_tasd_result_t chirbot_controller_tasd_decode(
    const uint8_t *document,
    size_t document_length,
    chirbot_controller_state_t *state)
{
    chirbot_controller_state_t decoded = {0};
    tasd_header_t header;
    tasd_reader_t reader;
    tasd_packet_t packet;
    tasd_result_t result;
    uint32_t expected_input_size;
    uint8_t controller_port = 0u;
    uint8_t input_port = 0u;
    int have_controller = 0;
    int have_input = 0;

    if (document == NULL || state == NULL) {
        return CHIRBOT_CONTROLLER_TASD_ERR_ARGUMENT;
    }
    if (tasd_read_header(document, document_length, &header) != TASD_OK ||
        header.version != TASD_VERSION || header.g_keylen != TASD_G_KEYLEN) {
        return CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED;
    }

    tasd_reader_init(&reader, document, document_length, &header);
    while ((result = tasd_reader_next(&reader, &packet)) == TASD_OK) {
        if (packet.key == TASD_KEY_PORT_CONTROLLER) {
            tasd_pkt_port_controller_t controller;
            if (have_controller ||
                tasd_decode_port_controller(&packet, &controller) != TASD_OK) {
                return CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED;
            }
            controller_port = controller.port;
            decoded.controller_type = controller.type;
            have_controller = 1;
        } else if (packet.key == TASD_KEY_INPUT_MOMENT) {
            tasd_pkt_input_moment_t input;
            if (have_input || tasd_decode_input_moment(&packet, &input) != TASD_OK ||
                input.inputs_len > CHIRBOT_CONTROLLER_MAX_INPUT_SIZE) {
                return CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED;
            }
            decoded.index = input.index;
            decoded.inputs_length = (uint8_t)input.inputs_len;
            memcpy(decoded.inputs, input.inputs, input.inputs_len);
            input_port = input.port;
            have_input = 1;
        }
    }

    if (result != TASD_ERR_END) {
        return CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED;
    }
    if (!have_controller || !have_input) {
        return CHIRBOT_CONTROLLER_TASD_ERR_INCOMPLETE;
    }
    if (controller_port != input_port) {
        return CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED;
    }

    decoded.port = controller_port;
    expected_input_size = supported_input_size(decoded.controller_type);
    if (expected_input_size == 0u) {
        return CHIRBOT_CONTROLLER_TASD_ERR_UNSUPPORTED;
    }
    if (decoded.inputs_length != expected_input_size) {
        return CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED;
    }

    *state = decoded;
    return CHIRBOT_CONTROLLER_TASD_OK;
}