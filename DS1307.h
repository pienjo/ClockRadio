#ifndef __DS1307_H__
#define __DS1307_H__

#include <inttypes.h>

void Read_DS1307_DateTime();
void Write_DS1307_DateTime();
void Init_DS1307();
void Read_DS1307_RAM(uint8_t *data, uint8_t addr, uint8_t size);
void Write_DS1307_RAM(uint8_t *data, uint8_t addr, uint8_t size);

#endif

