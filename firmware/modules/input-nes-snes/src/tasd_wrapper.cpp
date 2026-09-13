#include "nes_input_module.hpp"
#include "chirbot/controller_tasd.h"
#include "chirbot/link_protocol.h"
#include "pico/time.h"
#include <tasd.h>

bool NESInputModule::encode_tasd_packet(uint16_t controller_data) {
    uint8_t tasd_document[CHIRBOT_LINK_PAYLOAD_SIZE];
    uint16_t tasd_length = 0;
    chirbot_controller_state_t state = {};
    state.port = 0;
    state.controller_type = TASD_CTRL_NES_STANDARD;
    state.inputs[0] = (uint8_t)controller_data;
    state.inputs_length = 1;
    state.index = time_us_64() / 1000u;

    if (chirbot_controller_tasd_encode(&state, tasd_document,
                                       sizeof(tasd_document), &tasd_length) !=
        CHIRBOT_CONTROLLER_TASD_OK) {
        return false;
    }

    const uint32_t next_sequence = sequence_ + 1u;
    if (chirbot_link_encode(tx_frame_, CHIRBOT_LINK_FRAME_TASD, next_sequence,
                            tasd_document, tasd_length) != CHIRBOT_LINK_OK) {
        return false;
    }

    sequence_ = next_sequence;
    return true;
}
