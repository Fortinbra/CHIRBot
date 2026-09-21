#include "sd_storage.hpp"

#include "sd_probe.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace chirbot::sd {
namespace {

FATFS filesystem;
bool mounted = false;

void print_result(const char *operation, FRESULT result)
{
    if (result != FR_OK) {
        std::printf("SD: %s failed (%d)\r\n", operation, static_cast<int>(result));
    }
}

bool has_tasd_extension(const char *name)
{
    const size_t name_length = std::strlen(name);
    if (name_length < 5u) {
        return false;
    }
    char extension[5];
    for (size_t index = 0; index < 5u; ++index) {
        const char character = name[name_length - 5u + index];
        extension[index] = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return std::memcmp(extension, ".tasd", 5u) == 0;
}

bool read_open_file(FIL &file, uint8_t *buffer, size_t capacity, size_t *out_length)
{
    UINT bytes_read = 0;
    const FRESULT result = f_read(&file, buffer, static_cast<UINT>(capacity), &bytes_read);
    f_close(&file);
    if (result != FR_OK) {
        return false;
    }
    *out_length = bytes_read;
    return true;
}
}  // namespace

bool mount_filesystem()
{
    const FRESULT result = f_mount(&filesystem, "", 1);
    mounted = result == FR_OK;
    print_result("mount", result);
    if (mounted) {
        std::printf("SD: FAT filesystem mounted read/write\r\n");
    }
    return mounted;
}

void print_directory()
{
    if (!mounted) {
        return;
    }

    DIR directory;
    FILINFO file_info;
    FRESULT result = f_opendir(&directory, "");
    if (result != FR_OK) {
        print_result("open root directory", result);
        return;
    }

    std::printf("SD: FATFS root directory:\r\n");
    size_t listed = 0;
    while (listed < 128u) {
        result = f_readdir(&directory, &file_info);
        if (result != FR_OK || file_info.fname[0] == '\0') {
            break;
        }
        std::printf("SD:   %-40s %c %llu bytes\r\n", file_info.fname,
                    (file_info.fattrib & AM_DIR) != 0u ? 'D' : 'F',
                    static_cast<unsigned long long>(file_info.fsize));
        ++listed;
    }
    f_closedir(&directory);
    if (result != FR_OK) {
        print_result("read root directory", result);
    }
    std::printf("SD: FATFS listed %u root entries\r\n", static_cast<unsigned>(listed));
}

bool format_card()
{
    static uint8_t work[4096];
    MKFS_PARM options{};
    options.fmt = FM_FAT32;
    options.n_fat = 1;
    options.align = 0;
    options.n_root = 0;
    options.au_size = 0;

    std::printf("SD: formatting FAT32; this erases the card\r\n");
    const FRESULT result = f_mkfs("", &options, work, sizeof(work));
    print_result("format", result);
    if (result != FR_OK) {
        return false;
    }

    f_mount(nullptr, "", 0);
    mounted = false;
    return mount_filesystem();
}

bool read_first_tasd_file(uint8_t *buffer, size_t capacity, size_t *out_length,
                          size_t *out_file_size, char *name_out, size_t name_capacity)
{
    if (!mounted) {
        return false;
    }

    DIR directory;
    if (f_opendir(&directory, "") != FR_OK) {
        return false;
    }

    FILINFO file_info;
    bool found = false;
    while (f_readdir(&directory, &file_info) == FR_OK && file_info.fname[0] != '\0') {
        if ((file_info.fattrib & AM_DIR) == 0u && has_tasd_extension(file_info.fname)) {
            found = true;
            break;
        }
    }
    f_closedir(&directory);
    if (!found) {
        return false;
    }

    std::strncpy(name_out, file_info.fname, name_capacity - 1u);
    name_out[name_capacity - 1u] = '\0';
    *out_file_size = file_info.fsize;

    FIL file;
    if (f_open(&file, file_info.fname, FA_READ) != FR_OK) {
        return false;
    }
    return read_open_file(file, buffer, capacity, out_length);
}

size_t list_tasd_files(TasdFileEntry *out_entries, size_t max_entries)
{
    if (!mounted) {
        return 0u;
    }

    DIR directory;
    if (f_opendir(&directory, "") != FR_OK) {
        return 0u;
    }

    FILINFO file_info;
    size_t count = 0;
    while (count < max_entries &&
           f_readdir(&directory, &file_info) == FR_OK && file_info.fname[0] != '\0') {
        if ((file_info.fattrib & AM_DIR) != 0u || !has_tasd_extension(file_info.fname)) {
            continue;
        }
        std::strncpy(out_entries[count].name, file_info.fname,
                    sizeof(out_entries[count].name) - 1u);
        out_entries[count].name[sizeof(out_entries[count].name) - 1u] = '\0';
        out_entries[count].size_bytes = file_info.fsize;
        ++count;
    }
    f_closedir(&directory);
    return count;
}

bool read_tasd_file(const char *name, uint8_t *buffer, size_t capacity,
                    size_t *out_length, size_t *out_file_size)
{
    if (!mounted) {
        return false;
    }

    FILINFO file_info;
    if (f_stat(name, &file_info) != FR_OK) {
        return false;
    }
    *out_file_size = file_info.fsize;

    FIL file;
    if (f_open(&file, name, FA_READ) != FR_OK) {
        return false;
    }
    return read_open_file(file, buffer, capacity, out_length);
}

}  // namespace chirbot::sd

extern "C" {

#include "diskio.h"

static chirbot::sd::CardInfo g_card;

DSTATUS disk_initialize(BYTE drive)
{
    if (drive != 0u) {
        return STA_NOINIT;
    }
    g_card = chirbot::sd::probe();
    return g_card.initialized ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE drive)
{
    return drive == 0u && g_card.initialized ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count)
{
    if (drive != 0u || !g_card.initialized || count == 0u) {
        return RES_PARERR;
    }
    if (!chirbot::sd::read_blocks(static_cast<uint32_t>(sector), buffer, count,
                                  g_card.high_capacity)) {
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count)
{
    if (drive != 0u || !g_card.initialized || count == 0u) {
        return RES_PARERR;
    }
    if (!chirbot::sd::write_blocks(static_cast<uint32_t>(sector), buffer, count,
                                   g_card.high_capacity)) {
        std::printf("SD: format write failed at LBA %lu\r\n",
                    static_cast<unsigned long>(sector));
        return RES_ERROR;
    }
    if ((static_cast<uint32_t>(sector) & 0x7fu) == 0u) {
        std::printf("SD: format write reached LBA %lu\r\n",
                    static_cast<unsigned long>(sector));
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
    if (drive != 0u || !g_card.initialized) {
        return RES_NOTRDY;
    }
    switch (command) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *static_cast<LBA_t *>(buffer) = static_cast<LBA_t>(g_card.capacity_bytes / 512u);
        return RES_OK;
    case GET_SECTOR_SIZE:
        *static_cast<WORD *>(buffer) = 512u;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *static_cast<DWORD *>(buffer) = 1u;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}

}  // extern "C"