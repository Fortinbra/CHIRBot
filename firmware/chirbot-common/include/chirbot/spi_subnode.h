#ifndef CHIRBOT_SPI_SUBNODE_H
#define CHIRBOT_SPI_SUBNODE_H

#include <stddef.h>
#include <stdint.h>

#include "hardware/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Frame-aligned receive for module-link SPI subnodes.
 *
 * A PL022 subnode has no inherent frame alignment: it shifts whatever the main
 * node clocks, so a single spurious or dropped byte offsets every subsequent
 * frame permanently. Chip select is the only real frame delimiter, so each
 * transfer begins by waiting for the idle (CS high) gap between transactions
 * and flushing both FIFOs. That bounds any desync to the frame it occurred in.
 */

/* Waits for CS idle, then discards queued receive and transmit bytes. */
void chirbot_spi_subnode_resync(spi_inst_t *spi, uint cs_pin);

/* Realigns, then exchanges exactly `length` bytes with the main node. */
void chirbot_spi_subnode_transfer(
    spi_inst_t *spi,
    uint cs_pin,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length);

#ifdef __cplusplus
}
#endif

#endif
