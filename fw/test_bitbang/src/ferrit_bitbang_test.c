/*
 * FERRIT Bit-bang Diagnostic
 * Minimal SPI using gpio_get/gpio_put — no PIO, no hardware SPI.
 * Prints every bit received so we can see exactly what the chip returns.
 */

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "../../ferrit/src/ferrit.h"

#define HALF_PERIOD_US  10   // 50 kHz — very slow, easy to scope

static void sck_low(void)  { gpio_put(F_SCK,  0); sleep_us(HALF_PERIOD_US); }
static void sck_high(void) { gpio_put(F_SCK,  1); sleep_us(HALF_PERIOD_US); }
static void cs_assert(void)   { gpio_put(F_SS, 0); sleep_us(5); }
static void cs_deassert(void) { gpio_put(F_SS, 1); sleep_us(5); }

// Send one byte MSB first, return received byte
static uint8_t spi_byte(uint8_t tx) {
    uint8_t rx = 0;
    for (int i = 7; i >= 0; i--) {
        gpio_put(F_MOSI, (tx >> i) & 1);
        sck_low();
        sck_high();
        rx = (rx << 1) | gpio_get(F_MISO);
    }
    sck_low();
    return rx;
}

static void set_ss(int ss) {
    gpio_put(F_SS0, (ss >> 0) & 1);
    gpio_put(F_SS1, (ss >> 1) & 1);
    gpio_put(F_SS2, (ss >> 2) & 1);
    gpio_put(F_SS3, (ss >> 3) & 1);
}

static void set_cs(int cs) {
    gpio_put(F_CS0, (cs >> 0) & 1);
    gpio_put(F_CS1, (cs >> 1) & 1);
    gpio_put(F_CS2, (cs >> 2) & 1);
    gpio_put(F_CS3, (cs >> 3) & 1);
}

int main(void) {

    stdio_init_all();
    sleep_ms(3000);

    // Init all GPIOs as plain outputs/inputs
    gpio_init(F_SCK);  gpio_set_dir(F_SCK,  GPIO_OUT); gpio_put(F_SCK,  0);
    gpio_init(F_MOSI); gpio_set_dir(F_MOSI, GPIO_OUT); gpio_put(F_MOSI, 0);
    gpio_init(F_MISO); gpio_set_dir(F_MISO, GPIO_IN);
    gpio_pull_up(F_MISO);

    gpio_init(F_SS);  gpio_set_dir(F_SS,  GPIO_OUT); gpio_put(F_SS,  1);
    gpio_init(F_SS0); gpio_set_dir(F_SS0, GPIO_OUT); gpio_put(F_SS0, 0);
    gpio_init(F_SS1); gpio_set_dir(F_SS1, GPIO_OUT); gpio_put(F_SS1, 0);
    gpio_init(F_SS2); gpio_set_dir(F_SS2, GPIO_OUT); gpio_put(F_SS2, 0);
    gpio_init(F_SS3); gpio_set_dir(F_SS3, GPIO_OUT); gpio_put(F_SS3, 0);
    gpio_init(F_CS0); gpio_set_dir(F_CS0, GPIO_OUT); gpio_put(F_CS0, 0);
    gpio_init(F_CS1); gpio_set_dir(F_CS1, GPIO_OUT); gpio_put(F_CS1, 0);
    gpio_init(F_CS2); gpio_set_dir(F_CS2, GPIO_OUT); gpio_put(F_CS2, 0);
    gpio_init(F_CS3); gpio_set_dir(F_CS3, GPIO_OUT); gpio_put(F_CS3, 0);

    // WP and HOLD must be held high (no external pull-ups on board)
    gpio_init(F_SIO2); gpio_set_dir(F_SIO2, GPIO_OUT); gpio_put(F_SIO2, 1);  // WP high
    gpio_init(F_SIO3); gpio_set_dir(F_SIO3, GPIO_OUT); gpio_put(F_SIO3, 1);  // HOLD high

    printf("\n");
    printf("========================================\n");
    printf(" FERRIT Bit-bang Diagnostic\n");
    printf(" SCK=GPIO%d MOSI=GPIO%d MISO=GPIO%d\n", F_SCK, F_MOSI, F_MISO);
    printf("========================================\n\n");

    // 1. MISO idle level (no CS asserted)
    printf("MISO idle (no CS): %d  (expect 1 from pull-up)\n\n",
           gpio_get(F_MISO));

    // 2. RDID on slot 0, chip 0 — print every bit
    printf("Slot 0 Chip 0 RDID (0x9F + 4 dummy bytes):\n");

    set_ss(0);
    set_cs(0);
    sleep_us(2);
    cs_assert();

    printf("  F_SS asserted low -- measure F_SS (GPIO%d) and slot 0 demux output now.\n", F_SS);
    printf("  Holding for 15 seconds ...\n");
    fflush(stdout);
    sleep_ms(15000);
    printf("  Done holding.\n\n");

    printf("  MISO after CS assert: %d\n", gpio_get(F_MISO));

    uint8_t rdid_tx[5] = { 0x9F, 0x00, 0x00, 0x00, 0x00 };
    uint8_t rdid_rx[5] = { 0 };

    for (int b = 0; b < 5; b++) {
        rdid_rx[b] = spi_byte(rdid_tx[b]);
        printf("  byte[%d] tx=%02X rx=%02X\n", b, rdid_tx[b], rdid_rx[b]);
    }

    cs_deassert();

    printf("\nRDID result: %02X %02X %02X %02X\n",
           rdid_rx[1], rdid_rx[2], rdid_rx[3], rdid_rx[4]);
    printf("Expected:    04 7F 4A 81\n\n");

    // 3. MISO idle again after deselect
    printf("MISO idle (after CS): %d  (expect 1 from pull-up)\n\n",
           gpio_get(F_MISO));

    // 4. Raw MISO — clock 32 bits with no CS and print each bit
    printf("Raw MISO bits with CS deasserted (all should be 1):\n  ");
    for (int i = 0; i < 32; i++) {
        sck_low();
        sck_high();
        printf("%d", gpio_get(F_MISO));
        if ((i & 7) == 7) printf(" ");
    }
    sck_low();
    printf("\n\n");

    printf("Done.\n");

    while (1) tight_loop_contents();
}
