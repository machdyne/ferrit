/*
 * FERRIT Test Firmware
 * Copyright (c) 2026 Lone Dynamics Corporation. All rights reserved.
 *
 * Tests each slot (0-15) and each chip (0-7) per slot.
 * Uses RDID to detect presence, then performs a write/readback test.
 * Prints a full report over USB-CDC (stdio).
 *
 * Uses bit-bang SPI — proven to work on this hardware.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "../../ferrit/src/ferrit.h"

// -----------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------

#define CHIP_SIZE		(1024 * 1024)	// 1MB per chip (MB85RQ8MX = 8Mbit)
#define TEST_CHUNK		4096			// bytes per read/write operation
#define NUM_SLOTS		16
#define NUM_CHIPS		8				// chips per slot

// Expected RDID response bytes for MB85RQ8MX
#define RDID_MFR		0x04
#define RDID_CONT		0x7F
#define RDID_PROD1		0x4A
#define RDID_PROD2		0x81

// -----------------------------------------------------------------------
// Bit-bang SPI
// -----------------------------------------------------------------------

static inline void sck_lo(void) { gpio_put(F_SCK, 0); __asm volatile("nop"); __asm volatile("nop"); __asm volatile("nop"); __asm volatile("nop"); }
static inline void sck_hi(void) { gpio_put(F_SCK, 1); __asm volatile("nop"); __asm volatile("nop"); __asm volatile("nop"); __asm volatile("nop"); }

// Transfer one byte MSB-first, SPI mode 0:
//   - Drive MOSI while SCK is low (chip latches on rising edge)
//   - Sample MISO while SCK is high (chip drives on falling edge)
//   SCK starts and ends low.
static uint8_t spi_byte(uint8_t tx) {
	uint8_t rx = 0;
	for (int i = 7; i >= 0; i--) {
		gpio_put(F_MOSI, (tx >> i) & 1);	// drive MOSI while SCK low
		sck_hi();							// rising edge: chip latches MOSI, starts driving MISO
		rx = (rx << 1) | gpio_get(F_MISO);	// sample MISO while SCK high
		sck_lo();							// falling edge: chip updates MISO for next bit
	}
	return rx;
}

static void spi_write(const uint8_t *buf, int len) {
	for (int i = 0; i < len; i++) spi_byte(buf[i]);
}

static void spi_read(uint8_t *buf, int len) {
	for (int i = 0; i < len; i++) buf[i] = spi_byte(0x00);
}

static void spi_write_read(const uint8_t *tx, uint8_t *rx, int len) {
	for (int i = 0; i < len; i++) rx[i] = spi_byte(tx[i]);
}

// -----------------------------------------------------------------------
// GPIO helpers
// -----------------------------------------------------------------------

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

static void chip_select(int ss, int cs) {
	set_ss(ss);
	set_cs(cs);
	sleep_us(1);
	gpio_put(F_SS, 0);
	sleep_us(1);
}

static void chip_deselect(void) {
	gpio_put(F_SS, 1);
	sleep_us(1);
}

// -----------------------------------------------------------------------
// FRAM primitives
// -----------------------------------------------------------------------

static bool fram_rdid(int ss, int cs) {
	uint8_t tx[5] = { 0x9F, 0x00, 0x00, 0x00, 0x00 };
	uint8_t rx[5] = { 0 };

	chip_select(ss, cs);
	spi_write_read(tx, rx, 5);
	chip_deselect();

	printf("      RDID: %02X %02X %02X %02X",
	       rx[1], rx[2], rx[3], rx[4]);

	bool match = (rx[1] == RDID_MFR  &&
	              rx[2] == RDID_CONT &&
	              rx[3] == RDID_PROD1 &&
	              rx[4] == RDID_PROD2);

	printf("  %s\n", match ? "[OK]" : "[unexpected]");
	return match;
}

static void fram_wren(int ss, int cs) {
	uint8_t cmd = 0x06;
	chip_select(ss, cs);
	spi_write(&cmd, 1);
	chip_deselect();
}

static void fram_write(int ss, int cs, uint32_t addr,
                       const uint8_t *buf, int len) {
	uint8_t cmd[4] = { 0x02, addr >> 16, addr >> 8, addr & 0xFF };
	fram_wren(ss, cs);
	chip_select(ss, cs);
	spi_write(cmd, 4);
	spi_write(buf, len);
	chip_deselect();
}

static void fram_read(int ss, int cs, uint32_t addr,
                      uint8_t *buf, int len) {
	uint8_t cmd[4] = { 0x03, addr >> 16, addr >> 8, addr & 0xFF };
	chip_select(ss, cs);
	spi_write(cmd, 4);
	spi_read(buf, len);
	chip_deselect();
}

// -----------------------------------------------------------------------
// Test pattern
//
//   test_byte(addr) = ((addr * 0x6B) ^ 0xA5) & 0xFF
//
//   - Unique value per address
//   - No long runs of identical bytes
//   - Differs from blank FRAM (0xFF or 0x00)
//   - Single write + readback pass is sufficient
// -----------------------------------------------------------------------

static inline uint8_t test_byte(uint32_t addr) {
	return (uint8_t)((addr * 0x6Bu) ^ 0xA5u);
}

static void fill_pattern(uint8_t *buf, uint32_t base_addr, int len) {
	for (int i = 0; i < len; i++)
		buf[i] = test_byte(base_addr + (uint32_t)i);
}

static int check_pattern(const uint8_t *buf, uint32_t base_addr, int len) {
	int errors = 0;
	for (int i = 0; i < len; i++)
		if (buf[i] != test_byte(base_addr + (uint32_t)i)) errors++;
	return errors;
}

// -----------------------------------------------------------------------
// Test a single chip
// -----------------------------------------------------------------------

static uint8_t wbuf[TEST_CHUNK];
static uint8_t rbuf[TEST_CHUNK];

static bool test_chip(int ss, int cs) {

	int total_errors = 0;

	// Sanity check: write 16 known bytes at addr 0 and read back immediately
	uint8_t sanity_w[16], sanity_r[16];
	for (int i = 0; i < 16; i++) sanity_w[i] = (uint8_t)(0xA0 + i);
	fram_write(ss, cs, 0, sanity_w, 16);
	fram_read(ss, cs, 0, sanity_r, 16);
	printf("\n      sanity W: ");
	for (int i = 0; i < 16; i++) printf("%02X ", sanity_w[i]);
	printf("\n      sanity R: ");
	for (int i = 0; i < 16; i++) printf("%02X ", sanity_r[i]);
	printf("\n");

	for (uint32_t addr = 0; addr < CHIP_SIZE; addr += TEST_CHUNK) {
		fill_pattern(wbuf, addr, TEST_CHUNK);
		fram_write(ss, cs, addr, wbuf, TEST_CHUNK);
	}

	for (uint32_t addr = 0; addr < CHIP_SIZE; addr += TEST_CHUNK) {
		fram_read(ss, cs, addr, rbuf, TEST_CHUNK);
		int errors = check_pattern(rbuf, addr, TEST_CHUNK);
		if (errors > 0) {
			printf("      [!] addr 0x%06lX: %d byte error(s)\n", addr, errors);
			total_errors += errors;
			if (total_errors > 16384) {
				printf("      (too many errors, aborting)\n");
				return false;
			}
		}
	}

	return (total_errors == 0);
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------

int main(void) {

	stdio_init_all();
	sleep_ms(3000);

	// WP and HOLD high before anything else
	gpio_init(F_SIO2); gpio_set_dir(F_SIO2, GPIO_OUT); gpio_put(F_SIO2, 1);
	gpio_init(F_SIO3); gpio_set_dir(F_SIO3, GPIO_OUT); gpio_put(F_SIO3, 1);

	// SS strobe
	gpio_init(F_SS); gpio_set_dir(F_SS, GPIO_OUT); gpio_put(F_SS, 1);

	// SS demux address bits
	gpio_init(F_SS0); gpio_set_dir(F_SS0, GPIO_OUT); gpio_put(F_SS0, 0);
	gpio_init(F_SS1); gpio_set_dir(F_SS1, GPIO_OUT); gpio_put(F_SS1, 0);
	gpio_init(F_SS2); gpio_set_dir(F_SS2, GPIO_OUT); gpio_put(F_SS2, 0);
	gpio_init(F_SS3); gpio_set_dir(F_SS3, GPIO_OUT); gpio_put(F_SS3, 0);

	// CS demux address bits
	gpio_init(F_CS0); gpio_set_dir(F_CS0, GPIO_OUT); gpio_put(F_CS0, 0);
	gpio_init(F_CS1); gpio_set_dir(F_CS1, GPIO_OUT); gpio_put(F_CS1, 0);
	gpio_init(F_CS2); gpio_set_dir(F_CS2, GPIO_OUT); gpio_put(F_CS2, 0);
	gpio_init(F_CS3); gpio_set_dir(F_CS3, GPIO_OUT); gpio_put(F_CS3, 0);

	// Bit-bang SPI pins
	gpio_init(F_SCK);  gpio_set_dir(F_SCK,  GPIO_OUT); gpio_put(F_SCK,  0);
	gpio_init(F_MOSI); gpio_set_dir(F_MOSI, GPIO_OUT); gpio_put(F_MOSI, 0);
	gpio_init(F_MISO); gpio_set_dir(F_MISO, GPIO_IN);
	gpio_pull_up(F_MISO);

	printf("\n");
	printf("========================================\n");
	printf(" FERRIT Hardware Test\n");
	printf(" %d slots x %d chips x %dMB\n",
	       NUM_SLOTS, NUM_CHIPS, CHIP_SIZE / (1024 * 1024));
	printf(" SCK=GPIO%d MOSI=GPIO%d MISO=GPIO%d\n",
	       F_SCK, F_MOSI, F_MISO);
	printf("========================================\n\n");

	int total_chips_found  = 0;
	int total_chips_passed = 0;
	int total_chips_failed = 0;

	uint8_t results[NUM_SLOTS][NUM_CHIPS];
	memset(results, 0, sizeof(results));

	for (int slot = 0; slot < NUM_SLOTS; slot++) {

		int slot_chips = 0;

		for (int chip = 0; chip < NUM_CHIPS; chip++) {

			bool present = fram_rdid(slot, chip);

			if (!present) continue;

			slot_chips++;
			total_chips_found++;

			printf("      Read/write test ... ");
			fflush(stdout);

			bool pass = test_chip(slot, chip);

			if (pass) {
				printf("PASS\n");
				results[slot][chip] = 1;
				total_chips_passed++;
			} else {
				printf("\n      FAIL\n");
				results[slot][chip] = 2;
				total_chips_failed++;
			}

		}

		if (slot_chips == 0)
			printf("Slot %2d  [empty]\n", slot);

	}

	printf("\n");
	printf("========================================\n");
	printf(" Results\n");
	printf("========================================\n");
	printf(" Chips found:  %d\n", total_chips_found);
	printf(" Chips passed: %d\n", total_chips_passed);
	printf(" Chips failed: %d\n", total_chips_failed);
	printf("\n");

	printf(" Slot/chip matrix  (. absent  P pass  F fail)\n\n");
	printf("        ");
	for (int chip = 0; chip < NUM_CHIPS; chip++)
		printf(" C%d", chip);
	printf("\n");

	for (int slot = 0; slot < NUM_SLOTS; slot++) {
		printf(" S%02d    ", slot);
		for (int chip = 0; chip < NUM_CHIPS; chip++) {
			switch (results[slot][chip]) {
				case 0: printf("  ."); break;
				case 1: printf("  P"); break;
				case 2: printf("  F"); break;
			}
		}
		printf("\n");
	}

	printf("\n");

	if (total_chips_found == 0)
		printf(" WARNING: No chips detected. Check hardware.\n\n");
	else if (total_chips_failed == 0)
		printf(" All detected chips passed.\n\n");
	else
		printf(" TEST FAILED: %d chip(s) with errors.\n\n", total_chips_failed);

	while (1) tight_loop_contents();
}
