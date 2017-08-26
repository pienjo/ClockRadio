#ifndef __I2C_H__
#define __I2C_H__

#include <inttypes.h>

void Init_I2C();
void Read_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, uint8_t *ptr);
void Write_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, uint8_t *ptr);

#endif
