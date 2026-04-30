/*
 * FERRIT PIO SPI
 * Copyright (c) 2026 Lone Dynamics Corporation. All rights reserved.
 *
 * Full-duplex SPI mode 0 (CPOL=0, CPHA=0) using PIO.
 * Works on any three GPIOs regardless of hardware SPI peripheral mapping.
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ferrit_pio_spi.h"

void pio_spi_init(pio_spi_t *inst, PIO pio, uint sm,
                  uint pin_sck, uint pin_mosi, uint pin_miso,
                  float clk_div) {

    inst->pio = pio;
    inst->sm  = sm;

    // SPI mode 0, 2 instructions, sideset = SCK (1 bit, not optional):
    //   out pins, 1   side 0 [1]  -- shift MOSI out, SCK low,  delay 1
    //   in  pins, 1   side 1 [1]  -- sample MISO,    SCK high, delay 1
    uint16_t prog[2];
    prog[0] = pio_encode_out(pio_pins, 1) | pio_encode_sideset(1, 0) | pio_encode_delay(1);
    prog[1] = pio_encode_in(pio_pins,  1) | pio_encode_sideset(1, 1) | pio_encode_delay(1);

    struct pio_program p = {
        .instructions = prog,
        .length       = 2,
        .origin       = -1,
    };

    uint offset = pio_add_program(pio, &p);
    inst->offset = offset;

    // Hand all three pins to PIO before configuring the SM
    pio_gpio_init(pio, pin_sck);
    pio_gpio_init(pio, pin_mosi);
    pio_gpio_init(pio, pin_miso);

    // SCK and MOSI are outputs, MISO is input
    pio_sm_set_consecutive_pindirs(pio, sm, pin_sck,  1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_mosi, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_miso, 1, false);

    // Pull-up on MISO — no external pull-up on board
    gpio_pull_up(pin_miso);

    pio_sm_config c = pio_get_default_sm_config();

    sm_config_set_wrap(&c, offset, offset + 1);

    // 1 sideset bit, not optional, not pindirs
    sm_config_set_sideset(&c, 1, false, false);
    sm_config_set_sideset_pins(&c, pin_sck);

    // OUT drives MOSI, IN reads MISO
    sm_config_set_out_pins(&c, pin_mosi, 1);
    sm_config_set_in_pins(&c, pin_miso);

    // MSB first, autopull/autopush threshold = 8 bits
    // shift_direction=false means shift left = MSB first
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_in_shift(&c,  false, true, 8);

    sm_config_set_clkdiv(&c, clk_div);

    // Initialise SM — this sets SCK low via the sideset initial value
    pio_sm_init(pio, sm, offset, &c);

    // Force SCK low before enabling (sideset initial state)
    pio_sm_set_pins_with_mask(pio, sm, 0u, 1u << pin_sck);

    pio_sm_set_enabled(pio, sm, true);
}

void pio_spi_write_read_blocking(const pio_spi_t *inst,
                                 const uint8_t *src, uint8_t *dst,
                                 size_t len) {
    size_t tx_remain = len;
    size_t rx_remain = len;

    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(inst->pio, inst->sm)) {
            inst->pio->txf[inst->sm] = (uint32_t)(*src++) << 24;
            tx_remain--;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(inst->pio, inst->sm)) {
            *dst++ = (uint8_t)(inst->pio->rxf[inst->sm] >> 24);
            rx_remain--;
        }
    }
}

void pio_spi_write_blocking(const pio_spi_t *inst,
                            const uint8_t *src, size_t len) {
    size_t tx_remain = len;
    size_t rx_remain = len;

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
    size_t tx_remain = len;
    size_t rx_remain = len;

    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(inst->pio, inst->sm)) {
            inst->pio->txf[inst->sm] = tx_word;
            tx_remain--;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(inst->pio, inst->sm)) {
            *dst++ = (uint8_t)(inst->pio->rxf[inst->sm] >> 24);
            rx_remain--;
        }
    }
}
