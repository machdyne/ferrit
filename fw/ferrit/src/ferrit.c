/* 
 * FERRIT Firmware
 * Copyright (c) 2026 Lone Dynamics Corporation. All rights reserved.
 *
 */

#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ferrit.h"
#include "ferrit_pio_spi.h"

pio_spi_t ferrit_spi;

void ferrit_init(void) {

	// rgb led
	gpio_init(F_LEDR); gpio_set_dir(F_LEDR, true); gpio_put(F_LEDR, 1);
	gpio_init(F_LEDG); gpio_set_dir(F_LEDG, true); gpio_put(F_LEDG, 1);
	gpio_init(F_LEDB); gpio_set_dir(F_LEDB, true); gpio_put(F_LEDB, 1);

	// WP and HOLD must be driven high before any CS is asserted
	// (no external pull-ups on board)
	gpio_init(F_SIO2); gpio_set_dir(F_SIO2, GPIO_OUT); gpio_put(F_SIO2, 1);
	gpio_init(F_SIO3); gpio_set_dir(F_SIO3, GPIO_OUT); gpio_put(F_SIO3, 1);

	// SS strobe (active low enable on SN74HC154)
	gpio_init(F_SS); gpio_set_dir(F_SS, GPIO_OUT); gpio_put(F_SS, 1);

	// SS demux address bits (slot select 0-15)
	gpio_init(F_SS0); gpio_set_dir(F_SS0, GPIO_OUT); gpio_put(F_SS0, 0);
	gpio_init(F_SS1); gpio_set_dir(F_SS1, GPIO_OUT); gpio_put(F_SS1, 0);
	gpio_init(F_SS2); gpio_set_dir(F_SS2, GPIO_OUT); gpio_put(F_SS2, 0);
	gpio_init(F_SS3); gpio_set_dir(F_SS3, GPIO_OUT); gpio_put(F_SS3, 0);

	// CS demux address bits (chip select 0-15 on card)
	gpio_init(F_CS0); gpio_set_dir(F_CS0, GPIO_OUT); gpio_put(F_CS0, 0);
	gpio_init(F_CS1); gpio_set_dir(F_CS1, GPIO_OUT); gpio_put(F_CS1, 0);
	gpio_init(F_CS2); gpio_set_dir(F_CS2, GPIO_OUT); gpio_put(F_CS2, 0);
	gpio_init(F_CS3); gpio_set_dir(F_CS3, GPIO_OUT); gpio_put(F_CS3, 0);

	// PIO SPI — bypasses the SPI0/SPI1 GPIO mismatch on FERRIT-CY
	// clk_div=2 -> SCK ~7.8 MHz at 125MHz sys_clk
	pio_spi_init(&ferrit_spi, pio0, 0, F_SCK, F_MOSI, F_MISO, 2.0f);

}
