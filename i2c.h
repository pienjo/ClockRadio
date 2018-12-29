/*
Copyright 2018, Martijn van Buul <martijn.van.buul@gmail.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/
#ifndef __I2C_H__
#define __I2C_H__

#include <inttypes.h>

void Init_I2C();
_Bool Read_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, uint8_t *ptr);
_Bool Read_I2C_Raw(uint8_t addr, uint8_t amount, uint8_t *ptr);
_Bool Write_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, const uint8_t *ptr);
_Bool Write_I2C_Raw(uint8_t addr, uint8_t amount, const uint8_t *ptr);
#endif

