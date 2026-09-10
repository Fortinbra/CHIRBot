#pragma once

#include <stdint.h>

// SPI matrix protocol definitions
// Core (main) <-> Module (subnode) communication

#define SPI_PACKET_MAX_SIZE 64
#define SPI_HEADER_SIZE 4

typedef enum {
    SPI_CMD_READ_CONTROLLER = 0x01,
    SPI_CMD_WRITE_CONFIG = 0x02,
    SPI_CMD_HANDSHAKE = 0x03,
    SPI_CMD_ACK = 0x04
} spi_command_t;

typedef struct {
    uint32_t timestamp;
    uint16_t controller_data;
    uint8_t controller_type;
    uint8_t padding;
} spi_packet_t;
