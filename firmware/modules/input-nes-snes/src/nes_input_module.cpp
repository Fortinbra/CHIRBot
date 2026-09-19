#include "nes_input_module.hpp"
#include <pico/time.h>
#include <stdio.h>

namespace {

// NES standard controller bit layout used by the TASD input-moment payload.
constexpr uint8_t kBtnA = 0x01;
constexpr uint8_t kBtnB = 0x02;
constexpr uint8_t kBtnSelect = 0x04;
constexpr uint8_t kBtnStart = 0x08;
constexpr uint8_t kBtnUp = 0x10;
constexpr uint8_t kBtnDown = 0x20;
constexpr uint8_t kBtnLeft = 0x40;
constexpr uint8_t kBtnRight = 0x80;

// Proof-of-concept demo: replay the Konami code instead of sampling a real
// controller. Each entry is emitted as a full button press (hold + release).
// Remove once controller acquisition is brought up.
constexpr uint8_t kDemoSequence[] = {
    kBtnUp,   kBtnUp,    kBtnDown, kBtnDown,
    kBtnLeft, kBtnRight, kBtnLeft, kBtnRight,
    kBtnB,    kBtnA,     kBtnSelect, kBtnStart,
};
constexpr uint8_t kDemoStepCount = sizeof(kDemoSequence) / sizeof(kDemoSequence[0]);
constexpr uint64_t kDemoHoldUs = 2'500'000;
constexpr uint64_t kDemoReleaseUs = 500'000;

}  // namespace

NESInputModule::NESInputModule()
        : tx_frame_{},
            rx_frame_{},
            last_controller_data_(0),
            sequence_(0),
            frame_ready_(false),
            demo_index_(0),
            demo_held_(false),
            demo_next_step_us_(0) {
}

void NESInputModule::init() {
    setup_controller();
    setup_spi();
    
    printf("NES Input Module initialized (Konami demo)\n");
}

void NESInputModule::run() {
    const uint64_t now_us = time_us_64();
    if (!frame_ready_ || now_us >= demo_next_step_us_) {
        // Hold then release, so each step emits one complete button press.
        const uint8_t buttons = demo_held_ ? 0u : kDemoSequence[demo_index_];
        if (encode_tasd_packet(buttons)) {
            last_controller_data_ = buttons;
            frame_ready_ = true;
            if (demo_held_) {
                demo_index_ = (uint8_t)((demo_index_ + 1u) % kDemoStepCount);
                demo_held_ = false;
                demo_next_step_us_ = now_us + kDemoReleaseUs;
            } else {
                demo_held_ = true;
                demo_next_step_us_ = now_us + kDemoHoldUs;
            }
        }
    }

    handle_spi_request();
}
