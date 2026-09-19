#include "chirbot/spi_subnode.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/time.h"

/* Long enough to span several main-node poll intervals, short enough that a
 * wedged or unwired CS degrades to best-effort instead of hanging forever. */
#define CHIRBOT_SPI_SUBNODE_IDLE_TIMEOUT_US 5000u

bool chirbot_spi_subnode_resync(spi_inst_t *spi, uint cs_pin)
{
    spi_hw_t *const hw = spi_get_hw(spi);
    const uint64_t deadline = time_us_64() + CHIRBOT_SPI_SUBNODE_IDLE_TIMEOUT_US;
    bool idle = true;

    /* SIO still reports the pad level while the pin is muxed to the SPI
     * peripheral, so CS can be sampled without giving up hardware framing. */
    while (gpio_get(cs_pin) == 0) {
        if (time_us_64() > deadline) {
            idle = false;
            break;
        }
        tight_loop_contents();
    }

    while (spi_is_readable(spi)) {
        (void)hw->dr;
    }

    /* Toggling SSE is the only way to drop bytes already queued for transmit. */
    hw_clear_bits(&hw->cr1, SPI_SSPCR1_SSE_BITS);
    hw_set_bits(&hw->cr1, SPI_SSPCR1_SSE_BITS);

    return idle;
}

bool chirbot_spi_subnode_transfer(
    spi_inst_t *spi,
    uint cs_pin,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length)
{
    const bool idle = chirbot_spi_subnode_resync(spi, cs_pin);
    spi_write_read_blocking(spi, tx, rx, length);
    return idle;
}
