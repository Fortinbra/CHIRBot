#ifndef CHIRBOT_LINK_PROTOCOL_H
#define CHIRBOT_LINK_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIRBOT_LINK_FRAME_SIZE 64u
#define CHIRBOT_LINK_PAYLOAD_SIZE 50u
#define CHIRBOT_LINK_VERSION 1u

typedef enum {
    CHIRBOT_LINK_FRAME_EMPTY = 0,
    CHIRBOT_LINK_FRAME_TASD = 1
} chirbot_link_frame_type_t;

typedef enum {
    CHIRBOT_LINK_OK = 0,
    CHIRBOT_LINK_ERR_ARGUMENT = -1,
    CHIRBOT_LINK_ERR_MAGIC = -2,
    CHIRBOT_LINK_ERR_VERSION = -3,
    CHIRBOT_LINK_ERR_LENGTH = -4,
    CHIRBOT_LINK_ERR_CRC = -5
} chirbot_link_result_t;

typedef struct {
    uint8_t type;
    uint32_t sequence;
    uint16_t payload_length;
    const uint8_t *payload;
} chirbot_link_frame_view_t;

chirbot_link_result_t chirbot_link_encode(
    uint8_t frame[CHIRBOT_LINK_FRAME_SIZE],
    uint8_t type,
    uint32_t sequence,
    const uint8_t *payload,
    uint16_t payload_length);

chirbot_link_result_t chirbot_link_decode(
    const uint8_t frame[CHIRBOT_LINK_FRAME_SIZE],
    chirbot_link_frame_view_t *view);

#ifdef __cplusplus
}
#endif

#endif