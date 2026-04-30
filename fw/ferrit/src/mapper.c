/* 
 * FERRIT Mapper
 * Copyright (c) 2026 Lone Dynamics Corporation. All rights reserved.
 *
 * The map table associates each entry with a slot select (SS), chip select
 * (CS), LUN, driver type, and size. Multiple entries sharing the same LUN
 * are concatenated in order to form that LUN's address space.
 *
 * mapper_rec()            - find the map entry index for a LUN+address
 * mapper_get_local_addr() - translate a LUN address to a per-chip address
 *
 */

#include <stdint.h>
#include <stddef.h>

#include "ferrit.h"
#include "mapper.h"

const int f_map[256][5] = {

	// SS		CS		LUN		DRIVER			SIZE
	{ 0,		0,		0,		F_DRV_FRAM,		1048576 },
	{ 0,		1,		0,		F_DRV_FRAM,		1048576 },
	{ 0,		2,		0,		F_DRV_FRAM,		1048576 },
	{ 0,		3,		0,		F_DRV_FRAM,		1048576 },
	{ 0,		4,		0,		F_DRV_FRAM,		1048576 },
	{ 0,		5,		0,		F_DRV_FRAM,		1048576 },
	{ 0,		6,		0,		F_DRV_FRAM,		1048576 },
	{ 0,		7,		0,		F_DRV_FRAM,		1048576 },

	{ 0,		0,		0,		F_DRV_NONE,		0 },	// sentinel

};

// Returns the number of LUNs (highest LUN index + 1)
int mapper_luns(void) {

	int max_lun = 0;

	for (int i = 0; i < 256; i++) {
		if (f_map[i][3] == F_DRV_NONE) break;
		if (f_map[i][2] > max_lun) max_lun = f_map[i][2];
	}

	return (max_lun + 1);

}

// Returns the total size in bytes for a given LUN
int mapper_lun_size(uint8_t lun) {

	int size = 0;

	for (int i = 0; i < 256; i++) {
		if (f_map[i][3] == F_DRV_NONE) break;
		if (f_map[i][2] == lun) size += f_map[i][4];
	}

	return size;

}

// Returns the map entry index that owns the given LUN-absolute address,
// or -1 if not found.
int mapper_rec(uint8_t lun, uint32_t addr) {

	uint32_t p = 0;

	for (int i = 0; i < 256; i++) {
		if (f_map[i][3] == F_DRV_NONE) break;
		if (f_map[i][2] == lun) {
			uint32_t entry_end = p + (uint32_t)f_map[i][4];
			if (addr >= p && addr < entry_end)
				return i;
			p = entry_end;
		}
	}

	return -1;

}

// Returns how many bytes remain in the current chip from addr onwards.
// Used by callers to split transfers at chip boundaries.
int mapper_remaining(uint8_t lun, uint32_t addr) {

	uint32_t p = 0;

	for (int i = 0; i < 256; i++) {
		if (f_map[i][3] == F_DRV_NONE) break;
		if (f_map[i][2] == lun) {
			uint32_t entry_end = p + (uint32_t)f_map[i][4];
			if (addr >= p && addr < entry_end)
				return (int)(entry_end - addr);
			p = entry_end;
		}
	}

	return 0;

}

// Translates a LUN-absolute address to a chip-local address.
uint32_t mapper_get_local_addr(uint8_t lun, uint32_t addr) {

	uint32_t p = 0;

	for (int i = 0; i < 256; i++) {
		if (f_map[i][3] == F_DRV_NONE) break;
		if (f_map[i][2] == lun) {
			uint32_t entry_end = p + (uint32_t)f_map[i][4];
			if (addr >= p && addr < entry_end)
				return addr - p;
			p = entry_end;
		}
	}

	return 0;

}

// Returns the driver type for the chip owning the given LUN+address
int mapper_get_drv(uint8_t lun, uint32_t addr) {
	int rec = mapper_rec(lun, addr);
	if (rec < 0) return F_DRV_NONE;
	return f_map[rec][3];
}

// Returns the slot select (SS) number for the chip owning the given LUN+address
int mapper_get_ss(uint8_t lun, uint32_t addr) {
	int rec = mapper_rec(lun, addr);
	if (rec < 0) return -1;
	return f_map[rec][0];
}

// Returns the chip select (CS) number for the chip owning the given LUN+address
int mapper_get_cs(uint8_t lun, uint32_t addr) {
	int rec = mapper_rec(lun, addr);
	if (rec < 0) return -1;
	return f_map[rec][1];
}
