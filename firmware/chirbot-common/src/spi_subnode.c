#include "chirbot/spi_subnode.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"

void chirbot_spi_subnode_resync(spi_inst_t *spi, uint cs_pin)
{
    spi_hw_t *const hw = spi_get_hw(spi);

    /* SIO still reports the pad level while the pin is muxed to the SPI
     * peripheral, so CS can be sampled without giving up hardware framing. */
    while (gpio_get(cs_pin) == 0) {
        tight_loop_contents();
    }

    while (spi_is_readable(spi)) {
        (void)hw->dr;
    }

    /* Toggling SSE is the only way to drop bytes already queued for transmit. */
    hw_clear_bits(&hw->cr1, SPI_SSPCR1_SSE_BITS);
    hw_set_bits(&hw->cr1, SPI_SSPCR1_SSE_BITS);
}

void chirbot_spi_subnode_transfer(
    spi_inst_t *spi,
    uint cs_pin,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length)
{
    chirbot_spi_subnode_resync(spi, cs_pin);
    spi_write_read_blocking(spi, tx, rx, length);
}
