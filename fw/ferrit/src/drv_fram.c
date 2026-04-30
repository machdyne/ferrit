/* 
 * FERRIT FRAM driver
 * Copyright (c) 2026 Lone Dynamics Corporation. All rights reserved.
 *
 * SS (0-15) selects a card slot via a 4-bit demux on F_SS0..F_SS3.
 * CS (0-15) selects an IC on that card via a 4-bit demux on F_CS0..F_CS3.
 * F_SS is the strobe line used as the actual SPI chip select.
 *
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "ferrit.h"
#include "ferrit_pio_spi.h"
#include "drv_fram.h"

extern pio_spi_t ferrit_spi;

static void fram_set_ss(int ss) {
	gpio_put(F_SS0, (ss >> 0) & 1);
	gpio_put(F_SS1, (ss >> 1) & 1);
	gpio_put(F_SS2, (ss >> 2) & 1);
	gpio_put(F_SS3, (ss >> 3) & 1);
}

static void fram_set_cs(int cs) {
	gpio_put(F_CS0, (cs >> 0) & 1);
	gpio_put(F_CS1, (cs >> 1) & 1);
	gpio_put(F_CS2, (cs >> 2) & 1);
	gpio_put(F_CS3, (cs >> 3) & 1);
}

static void fram_select(int ss, int cs) {
	fram_set_ss(ss);
	fram_set_cs(cs);
	sleep_us(1);		// allow 74HC154/138 address lines to settle
	gpio_put(F_SS, 0);
	sleep_us(1);		// allow 74HC154/138 enable propagation
}

static void fram_deselect(void) {
	gpio_put(F_SS, 1);
	sleep_us(1);
}

void fram_init(void) {
	// PIO SPI initialised in ferrit_init()
}

void fram_read(int ss, int cs, char *buf, int addr, int len) {
	uint8_t cmd[4] = { 0x03, addr >> 16, addr >> 8, addr & 0xFF };
	fram_select(ss, cs);
	pio_spi_write_blocking(&ferrit_spi, cmd, 4);
	pio_spi_read_blocking(&ferrit_spi, 0x00, (uint8_t *)buf, len);
	fram_deselect();
}

static void fram_write_enable(int ss, int cs) {
	uint8_t cmd[1] = { 0x06 };
	fram_select(ss, cs);
	pio_spi_write_blocking(&ferrit_spi, cmd, 1);
	fram_deselect();
}

void fram_write(int ss, int cs, int addr, char *buf, int len) {
	uint8_t cmd[4] = { 0x02, addr >> 16, addr >> 8, addr & 0xFF };
	fram_write_enable(ss, cs);
	fram_select(ss, cs);
	pio_spi_write_blocking(&ferrit_spi, cmd, 4);
	pio_spi_write_blocking(&ferrit_spi, (uint8_t *)buf, len);
	fram_deselect();
}
