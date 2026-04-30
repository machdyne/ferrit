#ifndef _FERRIT_PIO_SPI_H_
#define _FERRIT_PIO_SPI_H_

#include <stdint.h>
#include "hardware/pio.h"

typedef struct {
    PIO     pio;
    uint    sm;
    uint    offset;
} pio_spi_t;

void pio_spi_init(pio_spi_t *spi, PIO pio, uint sm,
                  uint pin_sck, uint pin_mosi, uint pin_miso,
                  float clk_div);

void pio_spi_write_read_blocking(const pio_spi_t *spi,
                                 const uint8_t *src, uint8_t *dst,
                                 size_t len);

void pio_spi_write_blocking(const pio_spi_t *spi,
                            const uint8_t *src, size_t len);

void pio_spi_read_blocking(const pio_spi_t *spi,
                           uint8_t tx_byte, uint8_t *dst, size_t len);

#endif // _FERRIT_PIO_SPI_H_
