#pragma once

#include <cstdint>

#include "chirbot/link_protocol.h"

namespace chirbot::output {

constexpr uint8_t kLiveControllerPort = 0;
constexpr uint8_t kPlaybackControllerPort = 1;

constexpr bool accepts_controller_port(uint8_t frame_type, uint8_t port)
{
    return frame_type == CHIRBOT_LINK_FRAME_TASD
        ? port == kLiveControllerPort
        : (frame_type == CHIRBOT_LINK_FRAME_PLAYBACK_BEGIN ||
           frame_type == CHIRBOT_LINK_FRAME_PLAYBACK_STATE) &&
              port == kPlaybackControllerPort;
}

enum class ControllerProfile {
    Nes,
    Snes
};

constexpr uint32_t shift_word(ControllerProfile profile, uint16_t pressed)
{
    // NES shifts in low DATA after eight buttons. SNES uses pad inversion
    // so the PIO's zero fill becomes high DATA after sixteen buttons.
    return profile == ControllerProfile::Nes
               ? static_cast<uint32_t>(~pressed) & 0xFFu
               : static_cast<uint32_t>(pressed);
}

}  // namespace chirbot::output
