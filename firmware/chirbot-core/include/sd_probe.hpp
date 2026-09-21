#pragma once

#include <cstdint>

namespace chirbot::sd {

struct CardInfo {
    bool initialized;
    bool high_capacity;
    bool mmc;
    const char *failure_stage;
    uint8_t idle_miso;
    uint8_t cmd0_response;
    uint8_t cmd8_response;
    uint8_t cmd55_response;
    uint8_t acmd41_response;
    uint8_t ocr[4];
    uint8_t cid[16];
    uint8_t csd[16];
    uint64_t capacity_bytes;
    const char *filesystem;
    uint32_t filesystem_lba;
};

CardInfo probe();
bool read_blocks(uint32_t lba, uint8_t *output, uint32_t count, bool high_capacity);
bool write_blocks(uint32_t lba, const uint8_t *input, uint32_t count, bool high_capacity);
void print_info(const CardInfo &info);
void print_root_directory(const CardInfo &info);

}  // namespace chirbot::sd