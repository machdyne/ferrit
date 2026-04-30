/*
 * FERRIT PIO SPI
 * Copyright (c) 2026 Lone Dynamics Corporation. All rights reserved.
 *
 * Full-duplex SPI mode 0, MSB first, using PIO.
 * Program is in ferrit_spi.pio — pioasm generates ferrit_spi.pio.h.
 *
 * FIFO is 32-bit wide. With left-shift (MSB first) and autopush/pull at 8
 * bits, bytes must be written to txf as (byte << 24) and read from rxf
 * as (word & 0xFF) — ISR shifts right so received byte lands in bits [7:0].
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ferrit_pio_spi.h"

void pio_spi_init(pio_spi_t *inst, PIO pio, uint sm,
                  uint pin_sck, uint pin_mosi, uint pin_miso,
                  float clk_div) {
    inst->pio = pio;
    inst->sm  = sm;
    uint offset = pio_add_program(pio, &ferrit_spi_program);
    ferrit_spi_program_init(pio, sm, offset,
                            pin_sck, pin_mosi, pin_miso, clk_div);
}

void pio_spi_write_read_blocking(const pio_spi_t *inst,
                                 const uint8_t *src, uint8_t *dst,
                                 size_t len) {
    size_t tx_remain = len, rx_remain = len;
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(inst->pio, inst->sm)) {
            inst->pio->txf[inst->sm] = (uint32_t)(*src++) << 24;
            tx_remain--;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(inst->pio, inst->sm)) {
            *dst++ = (uint8_t)(inst->pio->rxf[inst->sm] & 0xFF);
            rx_remain--;
        }
    }
}

void pio_spi_write_blocking(const pio_spi_t *inst,
                            const uint8_t *src, size_t len) {
    size_t tx_remain = len, rx_remain = len;
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(inst->pio, inst->sm)) {
            inst->pio->txf[inst->sm] = (uint32_t)(*src++) << 24;
            tx_remain--;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(inst->pio, inst->sm)) {
            (void)inst->pio->rxf[inst->sm];
            rx_remain--;
        }
    }
}

void pio_spi_read_blocking(const pio_spi_t *inst,
                           uint8_t tx_byte, uint8_t *dst, size_t len) {
    uint32_t tx_word = (uint32_t)tx_byte << 24;
    size_t tx_remain = len, rx_remain = len;
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(inst->pio, inst->sm)) {
            inst->pio->txf[inst->sm] = tx_word;
            tx_remain--;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(inst->pio, inst->sm)) {
            *dst++ = (uint8_t)(inst->pio->rxf[inst->sm] & 0xFF);
            rx_remain--;
        }
    }
}
