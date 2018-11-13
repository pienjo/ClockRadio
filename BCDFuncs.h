#ifndef __BCDFUNCS_H__
#define __BCDFUNCS_H__

// Adds 2 BCD-encoded numbers. 
uint8_t BCDAdd(uint8_t left, uint8_t right);

// Subtracts 2 BCD-encoded numbers. 
uint8_t BCDSub(uint8_t left, uint8_t right);

// Converts a BCD-encoded number to binary
uint8_t BCDToBin(uint8_t bcd);

#define EDIT_MODE_ONES     0x1
#define EDIT_MODE_TENS     0x2
#define EDIT_MODE_MASK     0x03
#define EDIT_MODE_ONEBASE  0x04

// Perform key up handling
void HandleEditUp(const uint8_t editMode, uint8_t *const editDigit, const uint8_t editMaxValue);

// perform key down handling
void HandleEditDown(const uint8_t editMode, uint8_t *const editDigit, const uint8_t editMaxValue);

#endif
