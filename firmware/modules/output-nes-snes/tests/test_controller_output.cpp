#include "controller_output_state.hpp"
#include "chirbot/link_protocol.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using chirbot::output::ControllerProfile;
int failures = 0;

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

// Execute the production PIO source with the firmware's shift and pin mapping.
// Each tick models one instruction; pad synchronizers/analogue timing need a bench.
class ControllerPio {
public:
    explicit ControllerPio(const char *path, ControllerProfile profile,
                           uint16_t filter_us = 0, const char *timer_path = nullptr)
        : inverted_(profile == ControllerProfile::Snes), filter_us_(filter_us)
    {
        std::ifstream source(path);
        if (!source) {
            throw std::runtime_error("Cannot open production PIO source");
        }
        std::string line;
        while (std::getline(source, line)) {
            const auto begin = line.find_first_not_of(" \t\r");
            if (begin == std::string::npos || line[begin] == '.') {
                continue;
            }
            line = line.substr(begin);
            const auto end = line.find_last_not_of(" \t\r");
            line.resize(end + 1);
            if (line.back() == ':') {
                labels_[line.substr(0, line.size() - 1)] = instructions_.size();
                continue;
            }
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream words(line);
            std::vector<std::string> instruction;
            std::string word;
            while (words >> word) {
                instruction.push_back(word);
            }
            instructions_.push_back(instruction);
        }
        if (instructions_.empty() || instructions_.size() > 32) {
            throw std::runtime_error("Invalid PIO instruction memory size");
        }
        if (filter_us != 0u) {
            std::ifstream timer(timer_path);
            while (std::getline(timer, line)) {
                const auto begin = line.find_first_not_of(" \t\r");
                if (begin == std::string::npos || line[begin] == '.') {
                    continue;
                }
                line = line.substr(begin);
                if (line.find(':') != std::string::npos) {
                    continue;
                }
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream words(line);
                std::vector<std::string> instruction;
                std::string word;
                while (words >> word) {
                    instruction.push_back(word);
                }
                timer_instructions_.push_back(instruction);
            }
            check(timer_instructions_.size() == 4u &&
                      timer_instructions_.size() + instructions_.size() <= 32u,
                  "Production serializer and filter fit PIO instruction memory");
        }
        queue(chirbot::output::shift_word(profile, 0));
        settle();
    }

    void queue(uint32_t word)
    {
        fifo_.clear();
        fifo_.push_back(word);
    }

    bool append(uint32_t word)
    {
        if (fifo_.size() == CHIRBOT_LINK_PLAYBACK_QUEUE_CAPACITY) {
            return false;
        }
        fifo_.push_back(word);
        return true;
    }

    size_t pending() const { return fifo_.size(); }

    void advance_us(unsigned microseconds)
    {
        for (unsigned cycle = 0; cycle < microseconds * 150u; ++cycle) {
            tick();
        }
    }

    void signals(bool latch, bool clock)
    {
        latch_ = latch;
        clock_ = clock;
        settle();
    }

    bool data() const { return data_ != inverted_; }

    void settle()
    {
        for (unsigned cycle = 0; cycle < 64; ++cycle) {
            tick();
        }
    }

private:
    uint32_t value(const std::string &name) const
    {
        if (name == "osr") return osr_;
        if (name == "isr") return isr_;
        if (name == "null") return 0;
        if (name == "x") return x_;
        if (name == "status") return filter_us_ != 0u && irq_ ? UINT32_MAX : 0u;
        throw std::runtime_error("Unsupported PIO source operand: " + name);
    }

    void tick()
    {
        if (filter_us_ != 0u && ++timer_divider_ == 150u) {
            timer_divider_ = 0;
            const auto &timer_op = timer_instructions_.at(timer_pc_);
            if (timer_op.at(0) == "wait") {
                if (timer_op != std::vector<std::string>{"wait", "1", "irq", "1"}) {
                    throw std::runtime_error("Unsupported filter wait");
                }
                if (timer_request_) {
                    timer_request_ = false;
                    ++timer_pc_;
                }
            } else if (timer_op.at(0) == "mov") {
                check(timer_op == std::vector<std::string>{"mov", "y", "x"},
                      "Filter loads firmware-seeded countdown");
                timer_y_ = filter_us_ - 1u;
                ++timer_pc_;
            } else if (timer_op.at(0) == "jmp") {
                check(timer_op == std::vector<std::string>{"jmp", "y--", "delay"},
                      "Filter uses one-microsecond countdown ticks");
                if (timer_y_-- == 0u) {
                    ++timer_pc_;
                }
            } else if (timer_op == std::vector<std::string>{"irq", "clear", "0"}) {
                irq_ = false;
                timer_pc_ = 0;
            } else {
                throw std::runtime_error("Unsupported filter instruction");
            }
        }
        const auto &op = instructions_.at(pc_);
        size_t next = pc_ + 1;
        if (op.at(0) == "pull") {
            if (!fifo_.empty()) {
                osr_ = fifo_.front();
                fifo_.pop_front();
            } else if (op.at(1) == "noblock") {
                osr_ = x_;
            } else if (op.at(1) == "block") {
                return;
            } else {
                throw std::runtime_error("Unsupported PIO pull");
            }
        } else if (op.at(0) == "mov") {
            const uint32_t source = value(op.at(2));
            if (op.at(1) == "x") x_ = source;
            else if (op.at(1) == "osr") osr_ = source;
            else if (op.at(1) == "y") y_ = source;
            else if (op.at(1) == "isr") isr_ = source;
            else throw std::runtime_error("Unsupported PIO mov");
        } else if (op.at(0) == "out" && op.at(1) == "pins" && op.at(2) == "1") {
            data_ = (osr_ & 1u) != 0;
            osr_ >>= 1u;
        } else if (op.at(0) == "in" && op.at(1) == "pins" && op.at(2) == "1") {
            isr_ = (isr_ << 1u) | (clock_ ? 1u : 0u);
        } else if (op.at(0) == "irq" && op.at(1) == "0") {
            irq_ = true;
        } else if (op.at(0) == "irq" && op.at(1) == "1") {
            timer_request_ = true;
        } else if (op.at(0) == "jmp") {
            if (op.size() == 2) {
                next = labels_.at(op.at(1));
            } else if (op.at(1) == "pin") {
                if (latch_) next = labels_.at(op.at(2));
            } else if (op.at(1) == "!y") {
                if (y_ == 0) next = labels_.at(op.at(2));
            } else {
                throw std::runtime_error("Unsupported PIO jmp");
            }
        } else {
            throw std::runtime_error("Unsupported PIO instruction");
        }
        pc_ = next == instructions_.size() ? 0 : next;
    }

    std::vector<std::vector<std::string>> instructions_;
    std::map<std::string, size_t> labels_;
    std::deque<uint32_t> fifo_;
    size_t pc_ = 0;
    uint32_t x_ = 0;
    uint32_t y_ = 0;
    uint32_t osr_ = 0;
    uint32_t isr_ = 0;
    bool latch_ = false;
    bool clock_ = true;
    bool data_ = true;
    bool inverted_;
    uint16_t filter_us_ = 0;
    std::vector<std::vector<std::string>> timer_instructions_;
    size_t timer_pc_ = 0;
    unsigned timer_divider_ = 0;
    uint32_t timer_y_ = 0;
    bool irq_ = false;
    bool timer_request_ = false;
};

void latch_state(ControllerPio &pio, ControllerProfile profile, uint16_t pressed)
{
    pio.queue(chirbot::output::shift_word(profile, pressed));
    pio.signals(true, true);
    pio.signals(false, true);
}

void read_bits(ControllerPio &pio, ControllerProfile profile, uint16_t pressed,
               unsigned first = 0)
{
    const unsigned width = profile == ControllerProfile::Nes ? 8 : 16;
    for (unsigned bit = first; bit < 48; ++bit) {
        const bool expected = bit < width
                                  ? (pressed & (1u << bit)) == 0
                                  : profile == ControllerProfile::Snes;
        check(pio.data() == expected, "DATA before read / serial order / tail");
        pio.signals(false, false);
        check(pio.data() == expected, "DATA must remain stable on falling CLOCK");
        pio.signals(false, true);
    }
}

void test_serial(const char *path)
{
    ControllerPio nes(path, ControllerProfile::Nes);
    check(nes.data(), "NES starts released without a core transaction");
    for (unsigned pressed = 0; pressed <= 255; ++pressed) {
        const auto buttons = static_cast<uint16_t>(pressed);
        latch_state(nes, ControllerProfile::Nes, buttons);
        read_bits(nes, ControllerProfile::Nes, buttons);
    }
    latch_state(nes, ControllerProfile::Nes, 0x89);
    read_bits(nes, ControllerProfile::Nes, 0x89);
    nes.signals(true, true);
    nes.signals(false, true);
    read_bits(nes, ControllerProfile::Nes, 0x89);

    ControllerPio snes(path, ControllerProfile::Snes);
    check(snes.data(), "SNES starts released");
    for (unsigned bit = 0; bit < 16; ++bit) {
        const auto buttons = static_cast<uint16_t>(1u << bit);
        latch_state(snes, ControllerProfile::Snes, buttons);
        read_bits(snes, ControllerProfile::Snes, buttons);
    }
    for (const uint16_t buttons : {uint16_t{0}, uint16_t{0x4281}, uint16_t{0xFFFF}}) {
        latch_state(snes, ControllerProfile::Snes, buttons);
        read_bits(snes, ControllerProfile::Snes, buttons);
    }
}

void test_latch_and_updates(const char *path)
{
    ControllerPio pio(path, ControllerProfile::Nes);
    pio.signals(true, true);
    pio.queue(chirbot::output::shift_word(ControllerProfile::Nes, 1));
    pio.settle();
    check(!pio.data(), "A updates while LATCH is held high");
    for (unsigned read = 0; read < 12; ++read) {
        pio.signals(true, false);
        check(!pio.data(), "LATCH high keeps A on falling CLOCK");
        pio.signals(true, true);
        check(!pio.data(), "LATCH high keeps A on rising CLOCK");
    }
    pio.signals(false, true);
    pio.queue(chirbot::output::shift_word(ControllerProfile::Nes, 0x80));
    read_bits(pio, ControllerProfile::Nes, 1);
    pio.signals(true, true);
    pio.signals(false, true);
    read_bits(pio, ControllerProfile::Nes, 0x80);

    latch_state(pio, ControllerProfile::Nes, 0x89);
    pio.signals(false, false);
    pio.signals(false, true);
    pio.signals(false, false);
    pio.signals(true, false);
    check(!pio.data(), "Re-latch during an incomplete read restores A");
    pio.signals(true, true);
    pio.signals(false, true);
    read_bits(pio, ControllerProfile::Nes, 0x89);

    pio.queue(chirbot::output::shift_word(ControllerProfile::Nes, 0xFF));
    pio.queue(chirbot::output::shift_word(ControllerProfile::Nes, 0));
    pio.signals(true, true);
    pio.signals(false, true);
    read_bits(pio, ControllerProfile::Nes, 0);
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc != 4) {
        std::cerr << "Expected live, playback and latch-filter production PIO source paths\n";
        return 1;
    }
    try {
        for (unsigned port = 0; port <= UINT8_MAX; ++port) {
            const auto value = static_cast<uint8_t>(port);
            check(chirbot::output::accepts_controller_port(CHIRBOT_LINK_FRAME_TASD, value) ==
                      (port == 0u),
                  "Live input accepts only input-module port 0");
            check(chirbot::output::accepts_controller_port(CHIRBOT_LINK_FRAME_PLAYBACK_BEGIN, value) ==
                      (port == 1u),
                  "Playback begin accepts only movie player-1 port");
            check(chirbot::output::accepts_controller_port(CHIRBOT_LINK_FRAME_PLAYBACK_STATE, value) ==
                      (port == 1u),
                  "Playback state accepts only movie player-1 port");
            check(!chirbot::output::accepts_controller_port(CHIRBOT_LINK_FRAME_OUTPUT_STATUS, value),
                  "Non-state commands do not select a controller port");
        }
        test_serial(argv[1]);
        test_latch_and_updates(argv[1]);
        test_serial(argv[2]);
        for (const auto profile : {ControllerProfile::Nes, ControllerProfile::Snes}) {
            ControllerPio pio(argv[2], profile, 8000, argv[3]);
            pio.append(chirbot::output::shift_word(profile, 0x89));
            pio.append(chirbot::output::shift_word(profile, 0x02));
            pio.signals(true, true);
            pio.advance_us(16000);
            check(pio.pending() == 1u, "Held LATCH across expiry never consumes another report");
            pio.signals(false, true);
            read_bits(pio, profile, 0x89);
            pio.signals(true, true);
            pio.signals(false, true);
            check(pio.pending() == 0u, "LATCH edge after held-filter expiry consumes next report");
            read_bits(pio, profile, 0x02);
        }
        {
            ControllerPio pio(argv[2], ControllerProfile::Nes, 8000, argv[3]);
            pio.append(chirbot::output::shift_word(ControllerProfile::Nes, 1));
            pio.append(chirbot::output::shift_word(ControllerProfile::Nes, 2));
            pio.signals(true, true);
            pio.signals(false, true);
            pio.advance_us(7998);
            pio.signals(true, true);
            pio.signals(false, true);
            check(pio.pending() == 1u, "7998us is below the movie's 8000us threshold");
            pio.advance_us(5);
            pio.signals(true, true);
            pio.signals(false, true);
            check(pio.pending() == 0u, "Next poll after 8003us passes the filter");
        }
        for (const uint16_t filter : {uint16_t{1}, uint16_t{8000}, uint16_t{65535}}) {
            ControllerPio pio(argv[2], ControllerProfile::Nes, filter, argv[3]);
            pio.append(chirbot::output::shift_word(ControllerProfile::Nes, 0x89));
            pio.append(chirbot::output::shift_word(ControllerProfile::Nes, 0x02));
            pio.signals(true, true);
            pio.signals(false, true);
            check(pio.pending() == 1u, "First LATCH consumes first filtered report");
            if (filter == 8000u) {
                for (unsigned poll = 0; poll < 12u; ++poll) {
                    pio.advance_us(500);
                    pio.signals(true, true);
                    pio.signals(false, true);
                    read_bits(pio, ControllerProfile::Nes, 0x89);
                    check(pio.pending() == 1u, "Repeated polls inside 8ms retain complete report");
                }
            }
            pio.advance_us(filter + 5u);
            check(pio.pending() == 1u, "Filter expiry alone never consumes input");
            pio.signals(true, true);
            pio.signals(false, true);
            check(pio.pending() == 0u, "First LATCH after filter consumes next input");
            read_bits(pio, ControllerProfile::Nes, 0x02);
        }
        for (const auto profile : {ControllerProfile::Nes, ControllerProfile::Snes}) {
            ControllerPio pio(argv[2], profile);
            const uint16_t states[] = {0x01, 0x80, 0x09, 0};
            for (const auto state : states) {
                check(pio.append(chirbot::output::shift_word(profile, state)),
                      "Playback accepts four queued reports");
            }
            check(!pio.append(chirbot::output::shift_word(profile, 0xFF)),
                  "Full playback queue rejects rather than overwrites");
            pio.settle();
            check(pio.pending() == 4u && pio.data(),
                  "No console LATCH means no playback advancement");
            for (unsigned index = 0; index < 4u; ++index) {
                pio.signals(true, true);
                check(pio.pending() == 3u - index, "One report consumed per LATCH");
                for (unsigned clock = 0; clock < 12u; ++clock) {
                    pio.signals(true, false);
                    pio.signals(true, true);
                }
                check(pio.pending() == 3u - index,
                      "Held LATCH does not consume additional reports");
                pio.signals(false, true);
                read_bits(pio, profile, states[index]);
            }
            pio.signals(true, true);
            pio.signals(false, true);
            read_bits(pio, profile, 0);
            check(pio.pending() == 0u, "Empty queue repeats last state");

            check(pio.append(chirbot::output::shift_word(profile, 1)),
                  "Queue accepts refill after drain");
            pio.signals(true, true);
            pio.signals(false, true);
            pio.signals(false, false);
            pio.signals(false, true);
            check(pio.append(chirbot::output::shift_word(profile, 0x80)),
                  "Queue accepts update during read");
            read_bits(pio, profile, 1, 1);
            pio.signals(true, false);
            pio.signals(false, false);
            check(pio.pending() == 0u, "Interrupted read consumes next report on re-latch");
            check(pio.data(), "Re-latch starts next queued report at A");
        }
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    if (failures == 0) {
        std::cout << "Controller output waveform tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
