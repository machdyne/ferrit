#ifndef _DRV_FRAM_H_
#define _DRV_FRAM_H_

#include <stdbool.h>

void fram_init(void);
void fram_read(int ss, int cs, char *buf, int addr, int len);
void fram_write(int ss, int cs, int addr, char *buf, int len);

#endif	// _DRV_FRAM_H_
