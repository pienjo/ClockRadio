#ifndef __BCDFUNCS_H__
#define __BCDFUNCS_H__

// Adds 2 BCD-encoded numbers. 
uint8_t BCDAdd(uint8_t left, uint8_t right);

// Subtracts 2 BCD-encoded numbers. 
uint8_t BCDSub(uint8_t left, uint8_t right);

// Converts a BCD-encoded number to binary
uint8_t BCDToBin(uint8_t bcd);

#endif
