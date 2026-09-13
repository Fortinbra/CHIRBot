#include "chirbot/controller_tasd.h"
#include "chirbot/link_protocol.h"
#include "tasd.h"

#include <stdint.h>
#include <stdio.h>
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

static void check_pipeline(uint16_t controller_type,
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

    source.port = 0u;
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

int main(void)
{
    const uint8_t nes_inputs[] = {0x89u};
    const uint8_t snes_inputs[] = {0x81u, 0x42u};

    check_pipeline(TASD_CTRL_NES_STANDARD, nes_inputs, sizeof(nes_inputs));
    check_pipeline(TASD_CTRL_SNES_STANDARD, snes_inputs, sizeof(snes_inputs));
    test_rejections();
    test_reordered_port_mismatch();

    if (failures == 0) {
        puts("controller pipeline tests passed");
    }
    return failures == 0 ? 0 : 1;
}