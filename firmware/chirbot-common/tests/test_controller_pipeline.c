#include "chirbot/controller_tasd.h"
#include "chirbot/link_protocol.h"
#include "tasd.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(expression)                                                   \
    do {                                                                    \
        if (!(expression)) {                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,       \
                    #expression);                                           \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

static void check_pipeline(uint8_t port, uint16_t controller_type,
                           const uint8_t *inputs, uint8_t inputs_length)
{
    chirbot_controller_state_t source = {0};
    chirbot_controller_state_t output = {0};
    chirbot_link_frame_view_t core_view;
    chirbot_link_frame_view_t output_view;
    uint8_t document[CHIRBOT_LINK_PAYLOAD_SIZE];
    uint8_t input_frame[CHIRBOT_LINK_FRAME_SIZE];
    uint8_t forwarded_frame[CHIRBOT_LINK_FRAME_SIZE];
    uint16_t document_length = 0;

    source.port = port;
    source.controller_type = controller_type;
    source.inputs_length = inputs_length;
    source.index = 1234u;
    memcpy(source.inputs, inputs, inputs_length);

    CHECK(chirbot_controller_tasd_encode(
              &source, document, sizeof(document), &document_length) ==
          CHIRBOT_CONTROLLER_TASD_OK);
    CHECK(chirbot_link_encode(input_frame, CHIRBOT_LINK_FRAME_TASD, 17u,
                              document, document_length) == CHIRBOT_LINK_OK);

    CHECK(chirbot_link_decode(input_frame, &core_view) == CHIRBOT_LINK_OK);
    CHECK(core_view.type == CHIRBOT_LINK_FRAME_TASD);
    memcpy(forwarded_frame, input_frame, sizeof(forwarded_frame));

    CHECK(chirbot_link_decode(forwarded_frame, &output_view) == CHIRBOT_LINK_OK);
    CHECK(chirbot_controller_tasd_decode(output_view.payload,
                                         output_view.payload_length,
                                         &output) == CHIRBOT_CONTROLLER_TASD_OK);
    CHECK(output.port == source.port);
    CHECK(output.controller_type == source.controller_type);
    CHECK(output.inputs_length == source.inputs_length);
    CHECK(output.index == source.index);
    CHECK(memcmp(output.inputs, source.inputs, source.inputs_length) == 0);
}

static void test_rejections(void)
{
    chirbot_controller_state_t state = {0};
    chirbot_link_frame_view_t view;
    uint8_t document[CHIRBOT_LINK_PAYLOAD_SIZE];
    uint8_t frame[CHIRBOT_LINK_FRAME_SIZE];
    uint16_t document_length = 0;

    state.controller_type = TASD_CTRL_NES_STANDARD;
    state.inputs_length = 2u;
    CHECK(chirbot_controller_tasd_encode(
              &state, document, sizeof(document), &document_length) ==
          CHIRBOT_CONTROLLER_TASD_ERR_UNSUPPORTED);

    state.inputs_length = 1u;
    CHECK(chirbot_controller_tasd_encode(
              &state, document, sizeof(document), &document_length) ==
          CHIRBOT_CONTROLLER_TASD_OK);
    CHECK(chirbot_link_encode(frame, CHIRBOT_LINK_FRAME_TASD, 1u,
                              document, document_length) == CHIRBOT_LINK_OK);
    frame[10] ^= 0x01u;
    CHECK(chirbot_link_decode(frame, &view) == CHIRBOT_LINK_ERR_CRC);
}

static void test_standard_button_bytes(void)
{
    const uint8_t raw[] = {0xFFu, 0xEFu, 0x7Fu, 0xBFu, 0xDFu, 0xF7u,
                           0xFBu, 0xFDu, 0xFEu, 0xBEu, 0x00u};
    const uint8_t pressed[] = {0x00u, 0x08u, 0x01u, 0x02u, 0x04u, 0x10u,
                               0x20u, 0x40u, 0x80u, 0x82u, 0xFFu};
    for (size_t index = 0; index < sizeof(raw); ++index) {
        CHECK(chirbot_controller_tasd_input_byte(raw[index]) == pressed[index]);
        CHECK(chirbot_controller_tasd_input_byte(pressed[index]) == raw[index]);
    }
    for (unsigned value = 0; value <= UINT8_MAX; ++value) {
        const uint8_t buttons = (uint8_t)value;
        uint8_t document[CHIRBOT_LINK_PAYLOAD_SIZE];
        uint16_t length = 0;
        chirbot_controller_state_t state = {0};
        tasd_header_t header;
        tasd_reader_t reader;
        tasd_packet_t packet;
        tasd_pkt_input_moment_t moment;
        state.controller_type = TASD_CTRL_NES_STANDARD;
        state.inputs_length = 1u;
        state.inputs[0] = buttons;
        CHECK(chirbot_controller_tasd_encode(&state, document, sizeof(document), &length) ==
              CHIRBOT_CONTROLLER_TASD_OK);
        CHECK(tasd_read_header(document, length, &header) == TASD_OK);
        tasd_reader_init(&reader, document, length, &header);
        CHECK(tasd_reader_next(&reader, &packet) == TASD_OK);
        CHECK(tasd_reader_next(&reader, &packet) == TASD_OK);
        CHECK(tasd_decode_input_moment(&packet, &moment) == TASD_OK);
        CHECK(moment.inputs[0] == chirbot_controller_tasd_input_byte(buttons));
        CHECK(chirbot_controller_tasd_input_byte(moment.inputs[0]) == buttons);
        check_pipeline(0u, TASD_CTRL_NES_STANDARD, &buttons, 1u);
    }
    {
        const uint8_t snes[] = {0x82u, 0x09u};
        CHECK(chirbot_controller_tasd_input_byte(snes[0]) == 0xBEu);
        CHECK(chirbot_controller_tasd_input_byte(snes[1]) == 0x6Fu);
        check_pipeline(1u, TASD_CTRL_SNES_STANDARD, snes, 2u);
    }
}

static void test_reordered_port_mismatch(void)
{
    chirbot_controller_state_t decoded;
    tasd_pkt_input_moment_t input = {0};
    tasd_writer_t writer;
    uint8_t document[CHIRBOT_LINK_PAYLOAD_SIZE];
    uint8_t input_payload[12];
    uint8_t controller_payload[3];
    const uint8_t buttons = 0x01u;
    int input_length;
    int controller_length;

    input.port = 1u;
    input.index_type = TASD_INDEX_MILLISECONDS;
    input.inputs = &buttons;
    input.inputs_len = 1u;
    input_length = tasd_encode_input_moment(
        &input, input_payload, sizeof(input_payload));
    controller_length = tasd_encode_port_controller(
        0u, TASD_CTRL_NES_STANDARD,
        controller_payload, sizeof(controller_payload));

    tasd_writer_init(&writer, document, sizeof(document));
    CHECK(tasd_writer_write_header(&writer) == TASD_OK);
    CHECK(tasd_writer_append(&writer, TASD_KEY_INPUT_MOMENT,
                             input_payload, (uint32_t)input_length) == TASD_OK);
    CHECK(tasd_writer_append(&writer, TASD_KEY_PORT_CONTROLLER,
                             controller_payload,
                             (uint32_t)controller_length) == TASD_OK);
    CHECK(chirbot_controller_tasd_decode(
              document, tasd_writer_size(&writer), &decoded) ==
          CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED);
}

static void test_playback_frames(void)
{
    uint8_t frame[CHIRBOT_LINK_FRAME_SIZE];
    chirbot_link_frame_view_t view;
    const uint8_t status[] = {1u, CHIRBOT_LINK_PLAYBACK_QUEUE_CAPACITY};
    const uint8_t types[] = {
        CHIRBOT_LINK_FRAME_PLAYBACK_BEGIN,
        CHIRBOT_LINK_FRAME_PLAYBACK_STATE,
        CHIRBOT_LINK_FRAME_OUTPUT_STATUS,
        CHIRBOT_LINK_FRAME_PLAYBACK_STOP,
        CHIRBOT_LINK_FRAME_PLAYBACK_CONFIG
    };
    for (size_t index = 0; index < sizeof(types); ++index) {
        CHECK(chirbot_link_encode(frame, types[index], UINT32_MAX,
                                  status, sizeof(status)) == CHIRBOT_LINK_OK);
        CHECK(chirbot_link_decode(frame, &view) == CHIRBOT_LINK_OK);
        CHECK(view.type == types[index]);
        CHECK(view.sequence == UINT32_MAX);
        CHECK(view.payload_length == sizeof(status));
        CHECK(memcmp(view.payload, status, sizeof(status)) == 0);
        frame[11] ^= 1u;
        CHECK(chirbot_link_decode(frame, &view) == CHIRBOT_LINK_ERR_CRC);
    }
    CHECK(chirbot_link_encode(frame, CHIRBOT_LINK_FRAME_PLAYBACK_STOP,
                              0u, NULL, 0u) == CHIRBOT_LINK_OK);
    CHECK(chirbot_link_decode(frame, &view) == CHIRBOT_LINK_OK);
    CHECK(view.payload_length == 0u);
}

static void test_two_player_movie(const char *path)
{
    FILE *file = fopen(path, "rb");
    uint8_t *document;
    long length;
    tasd_header_t header;
    tasd_reader_t reader;
    tasd_packet_t packet;
    tasd_result_t result;
    uint16_t player_one_type = TASD_CTRL_NES_STANDARD;
    uint32_t player_one_reports = 0;
    uint32_t player_two_reports = 0;
    uint16_t latch_filter_us = 0;
    uint8_t clock_filter_100ns = 0;
    CHECK(file != NULL);
    if (file == NULL) {
        return;
    }
    CHECK(fseek(file, 0, SEEK_END) == 0);
    length = ftell(file);
    CHECK(length > 0 && length <= 458752);
    if (length <= 0 || length > 458752) {
        fclose(file);
        return;
    }
    CHECK(fseek(file, 0, SEEK_SET) == 0);
    document = (uint8_t *)malloc((size_t)length);
    CHECK(document != NULL);
    if (document == NULL) {
        fclose(file);
        return;
    }
    CHECK(fread(document, 1u, (size_t)length, file) == (size_t)length);
    CHECK(fclose(file) == 0);
    CHECK(tasd_read_header(document, (size_t)length, &header) == TASD_OK);
    tasd_reader_init(&reader, document, (size_t)length, &header);
    while ((result = tasd_reader_next(&reader, &packet)) == TASD_OK) {
        if (packet.key == TASD_KEY_NES_LATCH_FILTER) {
            CHECK(packet.payload_len == 2u);
            if (packet.payload_len == 2u) {
                latch_filter_us = (uint16_t)(((uint16_t)packet.payload[0] << 8u) |
                                             packet.payload[1]);
            }
        } else if (packet.key == TASD_KEY_NES_CLOCK_FILTER) {
            CHECK(packet.payload_len == 1u);
            if (packet.payload_len == 1u) {
                clock_filter_100ns = packet.payload[0];
            }
        } else if (packet.key == TASD_KEY_PORT_CONTROLLER) {
            tasd_pkt_port_controller_t controller;
            CHECK(tasd_decode_port_controller(&packet, &controller) == TASD_OK);
            if (controller.port == 1u) {
                player_one_type = controller.type;
            }
        } else if (packet.key == TASD_KEY_INPUT_CHUNK) {
            tasd_pkt_input_chunk_t chunk;
            CHECK(tasd_decode_input_chunk(&packet, &chunk) == TASD_OK);
            if (chunk.port == 1u) {
                CHECK(player_one_type == TASD_CTRL_NES_STANDARD);
                CHECK(chunk.inputs_len == 1u);
                const uint8_t pressed = chirbot_controller_tasd_input_byte(chunk.inputs[0]);
                if (player_one_reports == 0u || player_one_reports == 56u) {
                    CHECK(chunk.inputs[0] == 0xEFu);
                    CHECK(pressed == 0x08u);
                } else if (player_one_reports < 56u) {
                    CHECK(chunk.inputs[0] == 0xFFu);
                    CHECK(pressed == 0u);
                }
                check_pipeline(1u, player_one_type, &pressed, 1u);
                ++player_one_reports;
            } else if (chunk.port == 2u) {
                ++player_two_reports;
            }
        }
    }
    CHECK(result == TASD_ERR_END);
    CHECK(player_one_reports == 36127u);
    CHECK(player_two_reports == 36127u);
    CHECK(latch_filter_us == 8000u);
    CHECK(clock_filter_100ns == 35u);
    free(document);
}

int main(int argc, char **argv)
{
    const uint8_t nes_inputs[] = {0x89u};
    const uint8_t snes_inputs[] = {0x81u, 0x42u};

    check_pipeline(0u, TASD_CTRL_NES_STANDARD, nes_inputs, sizeof(nes_inputs));
    check_pipeline(1u, TASD_CTRL_NES_STANDARD, nes_inputs, sizeof(nes_inputs));
    check_pipeline(2u, TASD_CTRL_NES_STANDARD, nes_inputs, sizeof(nes_inputs));
    check_pipeline(0u, TASD_CTRL_SNES_STANDARD, snes_inputs, sizeof(snes_inputs));
    test_rejections();
    test_standard_button_bytes();
    test_reordered_port_mismatch();
    test_playback_frames();
    if (argc == 2) {
        test_two_player_movie(argv[1]);
    }

    if (failures == 0) {
        puts("controller pipeline tests passed");
    }
    return failures == 0 ? 0 : 1;
}