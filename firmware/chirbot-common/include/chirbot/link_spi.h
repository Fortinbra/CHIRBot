#ifndef CHIRBOT_LINK_SPI_H
#define CHIRBOT_LINK_SPI_H

#include "hardware/spi.h"

/*
 * Shared electrical format for both module links. Every main node and subnode
 * must configure identically, so these live here rather than in per-target
 * config headers.
 *
 * CPHA=1 is required, not a preference. A PL022 subnode in Motorola SPI mode
 * with SPH=0 only captures the first byte of each chip-select assertion; the
 * remainder of a multi-byte frame is dropped and MISO idles high. Because the
 * link deliberately holds CS low for a whole 64-byte frame to delimit it,
 * SPH=1 is the only setting that permits continuous back-to-back bytes within
 * one assertion.
 */

#define CHIRBOT_LINK_SPI_BAUD 2000000u
#define CHIRBOT_LINK_SPI_DATA_BITS 8
#define CHIRBOT_LINK_SPI_CPOL SPI_CPOL_0
#define CHIRBOT_LINK_SPI_CPHA SPI_CPHA_1
#define CHIRBOT_LINK_SPI_ORDER SPI_MSB_FIRST

#endif
