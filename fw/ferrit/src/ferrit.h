#ifndef _FERRIT_H_
#define _FERRIT_H_

#include <stdbool.h>

void ferrit_init(void);

#define F_DRV_NONE		0
#define F_DRV_FRAM		1
#define F_DRV_FLASH		2
#define F_DRV_EEPROM		3

#define F_SPI		spi0

#define F_LEDR		26
#define F_LEDG		27
#define F_LEDB		28

// SS strobe - used as the actual SPI chip select after demux lines are set
#define F_SS		1

// SS demux address bits (select card slot 0-15)
#define F_SS0		5
#define F_SS1		21
#define F_SS2		22
#define F_SS3		23

// CS demux address bits (select IC on card 0-15)
#define F_CS0		12
#define F_CS1		13
#define F_CS2		14
#define F_CS3		15

#define F_SCK		6
#define F_MOSI		7
#define F_MISO		8
#define F_SIO2		10
#define F_SIO3		11

#endif	// _FERRIT_H_
