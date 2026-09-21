#include "sd_probe.hpp"

#include "chirbot_core_config.hpp"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <cstdio>
#include <cstring>

namespace chirbot::sd {
namespace {

constexpr uint8_t kIdleByte = 0xff;
constexpr uint32_t kCommandTimeoutBytes = 1000;
constexpr uint32_t kDataTimeoutBytes = 100000;

uint8_t transfer(uint8_t value)
{
    uint8_t received = 0;
    for (int bit = 7; bit >= 0; --bit) {
        gpio_put(kSdMosiPin, (value >> bit) & 1u);
        gpio_put(kSdClockPin, 1);
        busy_wait_us_32(2);
        received = static_cast<uint8_t>((received << 1) | gpio_get(kSdMisoPin));
        gpio_put(kSdClockPin, 0);
        busy_wait_us_32(2);
    }
    return received;
}

void clock_bytes(uint32_t count)
{
    while (count-- != 0u) {
        transfer(kIdleByte);
    }
}

void select_card(bool selected)
{
    gpio_put(kSdChipSelectPin, selected ? 0 : 1);
    if (!selected) {
        clock_bytes(1);
    }
}

uint8_t command(uint8_t index, uint32_t argument, uint8_t crc)
{
    transfer(static_cast<uint8_t>(0x40u | index));
    transfer(static_cast<uint8_t>(argument >> 24));
    transfer(static_cast<uint8_t>(argument >> 16));
    transfer(static_cast<uint8_t>(argument >> 8));
    transfer(static_cast<uint8_t>(argument));
    transfer(crc);

    for (uint32_t count = 0; count < kCommandTimeoutBytes; ++count) {
        const uint8_t response = transfer(kIdleByte);
        if ((response & 0x80u) == 0u) {
            return response;
        }
    }
    return 0xff;
}

bool read_register(uint8_t command_index, uint8_t *output, size_t length)
{
    select_card(true);
    if (command(command_index, 0, 0x01) != 0u) {
        select_card(false);
        return false;
    }

    uint8_t token = kIdleByte;
    for (uint32_t count = 0; count < kDataTimeoutBytes; ++count) {
        token = transfer(kIdleByte);
        if (token != kIdleByte) {
            break;
        }
    }
    if (token != 0xfeu) {
        select_card(false);
        return false;
    }

    for (size_t index = 0; index < length; ++index) {
        output[index] = transfer(kIdleByte);
    }
    transfer(kIdleByte);
    transfer(kIdleByte);
    select_card(false);
    return true;
}

bool read_block(uint32_t lba, bool high_capacity, uint8_t *output)
{
    const uint32_t argument = high_capacity ? lba : lba * 512u;
    select_card(true);
    if (command(17, argument, 0x01) != 0u) {
        select_card(false);
        return false;
    }

    uint8_t token = kIdleByte;
    for (uint32_t count = 0; count < kDataTimeoutBytes; ++count) {
        token = transfer(kIdleByte);
        if (token != kIdleByte) {
            break;
        }
    }
    if (token != 0xfeu) {
        select_card(false);
        return false;
    }

    for (size_t index = 0; index < 512u; ++index) {
        output[index] = transfer(kIdleByte);
    }
    transfer(kIdleByte);
    transfer(kIdleByte);
    select_card(false);
    return true;
}

bool write_block(uint32_t lba, bool high_capacity, const uint8_t *input)
{
    const uint32_t argument = high_capacity ? lba : lba * 512u;
    select_card(true);
    if (command(24, argument, 0x01) != 0u) {
        select_card(false);
        return false;
    }
    transfer(0xff);
    transfer(0xfeu);
    for (size_t index = 0; index < 512u; ++index) {
        transfer(input[index]);
    }
    transfer(0xff);
    transfer(0xff);
    if ((transfer(0xff) & 0x1fu) != 0x05u) {
        select_card(false);
        return false;
    }
    for (uint32_t count = 0; count < kDataTimeoutBytes; ++count) {
        if (transfer(0xff) == 0xffu) {
            select_card(false);
            return true;
        }
    }
    select_card(false);
    return false;
}

bool has_text(const uint8_t *data, size_t offset, const char *text, size_t length)
{
    for (size_t index = 0; index < length; ++index) {
        if (data[offset + index] != static_cast<uint8_t>(text[index])) {
            return false;
        }
    }
    return true;
}

uint32_t little_endian_u32(const uint8_t *data)
{
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

uint16_t little_endian_u16(const uint8_t *data)
{
    return static_cast<uint16_t>(data[0]) |
           static_cast<uint16_t>(data[1] << 8);
}

const char *detect_filesystem(const uint8_t *sector)
{
    if (has_text(sector, 3, "EXFAT   ", 8)) {
        return "exFAT";
    }
    if (has_text(sector, 82, "FAT32   ", 8)) {
        return "FAT32";
    }
    if (has_text(sector, 54, "FAT16   ", 8)) {
        return "FAT16";
    }
    if (has_text(sector, 54, "FAT12   ", 8)) {
        return "FAT12";
    }
    return nullptr;
}

void print_hex(const uint8_t *data, size_t length)
{
    for (size_t index = 0; index < length; ++index) {
        std::printf("%02x", data[index]);
    }
}

uint64_t parse_capacity(const uint8_t *csd, bool high_capacity)
{
    if (high_capacity) {
        const uint32_t c_size = ((static_cast<uint32_t>(csd[7]) & 0x3fu) << 16) |
                                (static_cast<uint32_t>(csd[8]) << 8) | csd[9];
        return static_cast<uint64_t>(c_size + 1u) * 512u * 1024u;
    }

    const uint32_t c_size = ((static_cast<uint32_t>(csd[6]) & 0x03u) << 10) |
                            (static_cast<uint32_t>(csd[7]) << 2) |
                            ((csd[8] >> 6) & 0x03u);
    const uint32_t c_size_mult = ((static_cast<uint32_t>(csd[9]) & 0x03u) << 1) |
                                 ((csd[10] >> 7) & 0x01u);
    const uint32_t read_bl_len = csd[5] & 0x0fu;
    const uint64_t block_count = static_cast<uint64_t>(c_size + 1u)
                                 << (c_size_mult + 2u);
    return block_count << read_bl_len;
}

}  // namespace

bool read_blocks(uint32_t lba, uint8_t *output, uint32_t count, bool high_capacity)
{
    while (count-- != 0u) {
        if (!read_block(lba++, high_capacity, output)) {
            return false;
        }
        output += 512u;
    }
    return true;
}

bool write_blocks(uint32_t lba, const uint8_t *input, uint32_t count, bool high_capacity)
{
    while (count-- != 0u) {
        if (!write_block(lba++, high_capacity, input)) {
            return false;
        }
        input += 512u;
    }
    return true;
}

CardInfo probe()
{
    CardInfo info{};

    gpio_init(kSdClockPin);
    gpio_set_dir(kSdClockPin, GPIO_OUT);
    gpio_put(kSdClockPin, 0);
    gpio_init(kSdMosiPin);
    gpio_set_dir(kSdMosiPin, GPIO_OUT);
    gpio_put(kSdMosiPin, 1);
    gpio_init(kSdMisoPin);
    gpio_set_dir(kSdMisoPin, GPIO_IN);
    gpio_pull_up(kSdMisoPin);
    gpio_init(kSdChipSelectPin);
    gpio_set_dir(kSdChipSelectPin, GPIO_OUT);
    gpio_put(kSdChipSelectPin, 1);

    info.idle_miso = gpio_get(kSdMisoPin);
    clock_bytes(10);
    select_card(true);
    const uint8_t cmd0 = command(0, 0, 0x95);
    info.cmd0_response = cmd0;
    select_card(false);
    if (cmd0 != 0x01u) {
        info.failure_stage = "CMD0";
        return info;
    }

    uint8_t r7[4] = {};
    select_card(true);
    const uint8_t cmd8 = command(8, 0x1aau, 0x87);
    for (uint8_t &value : r7) {
        value = transfer(kIdleByte);
    }
    select_card(false);
    info.cmd8_response = cmd8;
    const bool version_two = cmd8 == 0x01u && r7[2] == 0x01u && r7[3] == 0xaau;

    uint8_t ready_response = 0xff;
    for (uint32_t attempt = 0; attempt < 2000 && ready_response != 0u; ++attempt) {
        select_card(true);
        info.cmd55_response = command(55, 0, 0x01);
        select_card(false);

        select_card(true);
        ready_response = command(41, version_two ? 0x40000000u : 0u, 0x01);
        select_card(false);
        sleep_ms(5);
    }

    // Some older or marginal cards reject HCS while still reporting a
    // version-two CMD8 response. Retry the standard argument before giving up.
    if (ready_response != 0u && version_two) {
        for (uint32_t attempt = 0; attempt < 2000 && ready_response != 0u; ++attempt) {
            select_card(true);
            info.cmd55_response = command(55, 0, 0x01);
            select_card(false);

            select_card(true);
            ready_response = command(41, 0, 0x01);
            select_card(false);
            sleep_ms(5);
        }
    }

    info.acmd41_response = ready_response;
    if (ready_response != 0u) {
        // MMC cards use CMD1 instead of the SD-specific CMD55/ACMD41 pair.
        uint8_t cmd1_response = 0xff;
        for (uint32_t attempt = 0; attempt < 400 && cmd1_response != 0u; ++attempt) {
            select_card(true);
            cmd1_response = command(1, 0, 0x01);
            select_card(false);
            sleep_ms(5);
        }
        if (cmd1_response != 0u) {
            info.failure_stage = "ACMD41/CMD1";
            return info;
        }
        info.mmc = true;
    }

    select_card(true);
    if (command(58, 0, 0x01) != 0u) {
        select_card(false);
        info.failure_stage = "CMD58";
        return info;
    }
    for (uint8_t &value : info.ocr) {
        value = transfer(kIdleByte);
    }
    select_card(false);
    info.high_capacity = (info.ocr[0] & 0x40u) != 0u;

    if (!read_register(9, info.csd, sizeof(info.csd)) ||
        !read_register(10, info.cid, sizeof(info.cid))) {
        info.failure_stage = "CID/CSD";
        return info;
    }

    info.capacity_bytes = parse_capacity(info.csd, info.high_capacity);
    info.initialized = true;

    uint8_t sector[512] = {};
    if (!read_block(0, info.high_capacity, sector)) {
        info.filesystem = "unreadable boot sector";
        return info;
    }

    info.filesystem = detect_filesystem(sector);
    info.filesystem_lba = 0;
    if (sector[510] == 0x55u && sector[511] == 0xaau) {
        if (has_text(sector, 1, "EFI PART", 8)) {
            info.filesystem = "GPT partition table";
        } else {
            for (size_t entry = 0; entry < 4u; ++entry) {
                const size_t offset = 446u + entry * 16u;
                const uint8_t partition_type = sector[offset + 4u];
                const uint32_t first_lba = little_endian_u32(sector + offset + 8u);
                if (partition_type != 0u && first_lba != 0u) {
                    info.filesystem_lba = first_lba;
                    if (read_block(first_lba, info.high_capacity, sector)) {
                        info.filesystem = detect_filesystem(sector);
                    }
                    if (info.filesystem == nullptr) {
                        info.filesystem = "MBR partition table (filesystem unknown)";
                    }
                    break;
                }
            }
        }
    }
    if (info.filesystem == nullptr) {
        info.filesystem = "unknown or unformatted";
    }
    return info;
}

void print_info(const CardInfo &info)
{
    if (!info.initialized) {
        std::printf("SD: probe failed at %s; idle_miso=%u CMD0=0x%02x "
                "CMD8=0x%02x CMD55=0x%02x ACMD41=0x%02x\r\n",
                    info.failure_stage ? info.failure_stage : "unknown",
                info.idle_miso, info.cmd0_response, info.cmd8_response,
                info.cmd55_response, info.acmd41_response);
        return;
    }

    std::printf("SD: card detected, type=%s, capacity=%llu MiB\r\n",
                info.mmc ? "MMC" : (info.high_capacity ? "SDHC/SDXC" : "SDSC"),
                static_cast<unsigned long long>(info.capacity_bytes / (1024u * 1024u)));
    std::printf("SD: filesystem probe: %s (LBA %lu); read-only, not mounted\r\n",
                info.filesystem, static_cast<unsigned long>(info.filesystem_lba));
    std::printf("SD: OCR=");
    print_hex(info.ocr, sizeof(info.ocr));
    std::printf(" CID=");
    print_hex(info.cid, sizeof(info.cid));
    std::printf(" CSD=");
    print_hex(info.csd, sizeof(info.csd));
    std::printf("\r\n");
    std::printf("SD: raw card registers read successfully; filesystem was not mounted\r\n");
}

void print_root_directory(const CardInfo &info)
{
    if (!info.initialized || info.filesystem == nullptr ||
        std::strcmp(info.filesystem, "FAT32") != 0) {
        return;
    }

    uint8_t sector[512] = {};
    if (!read_block(info.filesystem_lba, info.high_capacity, sector)) {
        std::printf("SD: FAT32 boot sector read failed\r\n");
        return;
    }

    const uint16_t bytes_per_sector = little_endian_u16(sector + 11);
    const uint8_t sectors_per_cluster = sector[13];
    const uint16_t reserved_sectors = little_endian_u16(sector + 14);
    const uint8_t fat_count = sector[16];
    const uint32_t sectors_per_fat = little_endian_u32(sector + 36);
    const uint32_t root_cluster = little_endian_u32(sector + 44);
    if (bytes_per_sector != 512u || sectors_per_cluster == 0u ||
        reserved_sectors == 0u || fat_count == 0u || sectors_per_fat == 0u ||
        root_cluster < 2u) {
        std::printf("SD: invalid FAT32 boot parameters\r\n");
        return;
    }

    const uint32_t fat_lba = info.filesystem_lba + reserved_sectors;
    const uint32_t data_lba = fat_lba + static_cast<uint32_t>(fat_count) * sectors_per_fat;
    uint32_t cluster = root_cluster;
    size_t listed = 0;
    std::printf("SD: FAT32 root directory (read-only):\r\n");

    for (size_t cluster_guard = 0; cluster_guard < 1024u; ++cluster_guard) {
        const uint32_t cluster_lba = data_lba + (cluster - 2u) * sectors_per_cluster;
        for (uint8_t sector_index = 0; sector_index < sectors_per_cluster; ++sector_index) {
            if (!read_block(cluster_lba + sector_index, info.high_capacity, sector)) {
                std::printf("SD: directory read failed at LBA %lu\r\n",
                            static_cast<unsigned long>(cluster_lba + sector_index));
                return;
            }

            for (size_t offset = 0; offset < 512u; offset += 32u) {
                const uint8_t first = sector[offset];
                if (first == 0x00u) {
                    std::printf("SD: listed %u root entries\r\n",
                                static_cast<unsigned>(listed));
                    return;
                }
                if (first == 0xe5u || sector[offset + 11u] == 0x0fu ||
                    (sector[offset + 11u] & 0x08u) != 0u) {
                    continue;
                }

                char name[13] = {};
                size_t name_length = 0;
                for (size_t index = 0; index < 8u && sector[offset + index] != ' '; ++index) {
                    name[name_length++] = static_cast<char>(sector[offset + index]);
                }
                if (sector[offset + 8u] != ' ') {
                    name[name_length++] = '.';
                    for (size_t index = 0; index < 3u && sector[offset + 8u + index] != ' '; ++index) {
                        name[name_length++] = static_cast<char>(sector[offset + 8u + index]);
                    }
                }
                name[name_length] = '\0';
                const uint32_t size = little_endian_u32(sector + offset + 28u);
                std::printf("SD:   %-12s %c %lu bytes\r\n", name,
                            (sector[offset + 11u] & 0x10u) != 0u ? 'D' : 'F',
                            static_cast<unsigned long>(size));
                if (++listed >= 128u) {
                    std::printf("SD: listed first 128 root entries\r\n");
                    return;
                }
            }
        }

        uint8_t fat_sector[512] = {};
        const uint32_t fat_entry_offset = cluster * 4u;
        if (!read_block(fat_lba + fat_entry_offset / 512u,
                        info.high_capacity, fat_sector)) {
            std::printf("SD: FAT read failed\r\n");
            return;
        }
        cluster = little_endian_u32(fat_sector + fat_entry_offset % 512u) & 0x0fffffffu;
        if (cluster >= 0x0ffffff8u) {
            std::printf("SD: listed %u root entries\r\n", static_cast<unsigned>(listed));
            return;
        }
        if (cluster < 2u) {
            std::printf("SD: invalid FAT32 cluster chain\r\n");
            return;
        }
    }
    std::printf("SD: root directory chain exceeded safety limit\r\n");
}

}  // namespace chirbot::sd