#include "chirbot/link_protocol.h"

#include <stddef.h>
#include <string.h>

#define MAGIC_0 'C'
#define MAGIC_1 'H'
#define HEADER_SIZE 10u
#define CRC_OFFSET 60u

static uint32_t crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8u; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }

    return ~crc;
}

static void write_be16(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t)(value >> 8u);
    destination[1] = (uint8_t)value;
}

static void write_be32(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)(value >> 24u);
    destination[1] = (uint8_t)(value >> 16u);
    destination[2] = (uint8_t)(value >> 8u);
    destination[3] = (uint8_t)value;
}

static uint16_t read_be16(const uint8_t *source)
{
    return (uint16_t)(((uint16_t)source[0] << 8u) | source[1]);
}

static uint32_t read_be32(const uint8_t *source)
{
    return ((uint32_t)source[0] << 24u) |
           ((uint32_t)source[1] << 16u) |
           ((uint32_t)source[2] << 8u) |
           (uint32_t)source[3];
}

chirbot_link_result_t chirbot_link_encode(
    uint8_t frame[CHIRBOT_LINK_FRAME_SIZE],
    uint8_t type,
    uint32_t sequence,
    const uint8_t *payload,
    uint16_t payload_length)
{
    if (frame == NULL || (payload_length > 0u && payload == NULL)) {
        return CHIRBOT_LINK_ERR_ARGUMENT;
    }
    if (payload_length > CHIRBOT_LINK_PAYLOAD_SIZE) {
        return CHIRBOT_LINK_ERR_LENGTH;
    }

    memset(frame, 0, CHIRBOT_LINK_FRAME_SIZE);
    frame[0] = MAGIC_0;
    frame[1] = MAGIC_1;
    frame[2] = CHIRBOT_LINK_VERSION;
    frame[3] = type;
    write_be32(frame + 4u, sequence);
    write_be16(frame + 8u, payload_length);
    if (payload_length > 0u) {
        memcpy(frame + HEADER_SIZE, payload, payload_length);
    }
    write_be32(frame + CRC_OFFSET, crc32(frame, CRC_OFFSET));

    return CHIRBOT_LINK_OK;
}

chirbot_link_result_t chirbot_link_decode(
    const uint8_t frame[CHIRBOT_LINK_FRAME_SIZE],
    chirbot_link_frame_view_t *view)
{
    uint16_t payload_length;

    if (frame == NULL || view == NULL) {
        return CHIRBOT_LINK_ERR_ARGUMENT;
    }
    if (frame[0] != MAGIC_0 || frame[1] != MAGIC_1) {
        return CHIRBOT_LINK_ERR_MAGIC;
    }
    if (frame[2] != CHIRBOT_LINK_VERSION) {
        return CHIRBOT_LINK_ERR_VERSION;
    }

    payload_length = read_be16(frame + 8u);
    if (payload_length > CHIRBOT_LINK_PAYLOAD_SIZE) {
        return CHIRBOT_LINK_ERR_LENGTH;
    }
    if (read_be32(frame + CRC_OFFSET) != crc32(frame, CRC_OFFSET)) {
        return CHIRBOT_LINK_ERR_CRC;
    }

    view->type = frame[3];
    view->sequence = read_be32(frame + 4u);
    view->payload_length = payload_length;
    view->payload = frame + HEADER_SIZE;
    return CHIRBOT_LINK_OK;
}