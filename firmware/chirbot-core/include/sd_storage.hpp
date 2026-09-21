#pragma once

#include "ff.h"

#include <cstddef>

namespace chirbot::sd {

bool mount_filesystem();
void print_directory();
bool format_card();

struct TasdFileEntry {
    char name[64];
    uint32_t size_bytes;
};

// Fills out_entries (capacity max_entries) with root-directory ".tasd" files.
// Returns the number found, capped at max_entries.
size_t list_tasd_files(TasdFileEntry *out_entries, size_t max_entries);

// Reads the named file from the root directory into buffer. Returns false if
// it does not exist or the read fails. *out_length is the number of bytes
// copied (may be less than *out_file_size if the file exceeds capacity).
bool read_tasd_file(const char *name, uint8_t *buffer, size_t capacity,
                    size_t *out_length, size_t *out_file_size);

// Reads the first ".tasd" file found in the root directory into buffer.
// Returns false if none is found, the read fails, or nothing was copied.
// *out_length is the number of bytes copied into buffer (may be less than
// *out_file_size if the file is larger than capacity).
bool read_first_tasd_file(uint8_t *buffer, size_t capacity, size_t *out_length,
                          size_t *out_file_size, char *name_out, size_t name_capacity);

}  // namespace chirbot::sd