#ifndef CHIRBOT_CONTROLLER_TASD_H
#define CHIRBOT_CONTROLLER_TASD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIRBOT_CONTROLLER_MAX_INPUT_SIZE 2u

typedef enum {
    CHIRBOT_CONTROLLER_TASD_OK = 0,
    CHIRBOT_CONTROLLER_TASD_ERR_ARGUMENT = -1,
    CHIRBOT_CONTROLLER_TASD_ERR_UNSUPPORTED = -2,
    CHIRBOT_CONTROLLER_TASD_ERR_MALFORMED = -3,
    CHIRBOT_CONTROLLER_TASD_ERR_BUFFER_SMALL = -4,
    CHIRBOT_CONTROLLER_TASD_ERR_INCOMPLETE = -5
} chirbot_controller_tasd_result_t;

typedef struct {
    uint8_t port;
    uint16_t controller_type;
    uint8_t inputs[CHIRBOT_CONTROLLER_MAX_INPUT_SIZE];
    uint8_t inputs_length;
    uint64_t index;
} chirbot_controller_state_t;

chirbot_controller_tasd_result_t chirbot_controller_tasd_encode(
    const chirbot_controller_state_t *state,
    uint8_t *document,
    size_t capacity,
    uint16_t *document_length);

chirbot_controller_tasd_result_t chirbot_controller_tasd_decode(
    const uint8_t *document,
    size_t document_length,
    chirbot_controller_state_t *state);

#ifdef __cplusplus
}
#endif

#endif