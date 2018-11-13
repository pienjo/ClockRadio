#include <stdint.h>
#include "BCDFuncs.h"


static inline uint8_t BCDAdd_digit(uint8_t left, uint8_t right) {
  
  uint8_t result = left + right; // max value after addition: 18.
  
  if (result >= 10)
  {
    result += 6;
  }  
  
  return result;
}

uint8_t BCDAdd(uint8_t left, uint8_t right)
{
  uint8_t ones = BCDAdd_digit(left & 0x0f, right & 0x0f);
  uint8_t tens = BCDAdd_digit(left >> 4, right >> 4) & 0x0f;
  tens = BCDAdd_digit(tens, ones >> 4); // carry
  
  return (tens << 4) + (ones & 0x0f);
}

static inline uint8_t BCDSub_digit(uint8_t left, uint8_t right) {
  
  uint8_t result = left - right; // Range after subtraction: 0-9, 247 - 255
  
  if (result >= 10)
  {
    result -= (-10 - 0x90);
  }  
  
  return result;
}

uint8_t BCDSub(uint8_t left, uint8_t right) 
{
  uint8_t ones = BCDSub_digit(left & 0x0f, right & 0x0f);
  uint8_t tens = BCDSub_digit(left >> 4, right >> 4) & 0x0f;
  tens = BCDAdd_digit(tens, ones >> 4); // carry
  
  return (tens << 4) + (ones & 0x0f);
}

uint8_t BCDToBin(uint8_t bcd)
{
  uint8_t bin = 0;
  while (bcd & 0xf0) {
    bin += 10;
    bcd -= 0x10;
  }
  
  bin += bcd;
  
  return bin;
}


void HandleEditUp(const uint8_t editMode, uint8_t *const editDigit, const uint8_t editMaxValue)
{
  if (!editDigit)
    return;
  switch(editMode & EDIT_MODE_MASK)
  {
    case EDIT_MODE_ONES:
    {
      *editDigit = BCDAdd(*editDigit, 1);
      break;
    }
    case EDIT_MODE_TENS:
    {
      *editDigit = BCDAdd(*editDigit, 0x10);
      break;
    }
  }
  
  if (*editDigit > editMaxValue)
  {
    if (editMode & EDIT_MODE_ONEBASE)
      *editDigit = editMaxValue;
    else
    {
      *editDigit = BCDSub( *editDigit, editMaxValue);
      *editDigit = BCDSub( *editDigit, 1);
    }
  }
}

void HandleEditDown(const uint8_t editMode, uint8_t *const editDigit, const uint8_t editMaxValue)
{
  switch(editMode & EDIT_MODE_MASK)
  {
    case EDIT_MODE_ONES:
    {
      *editDigit = BCDSub(*editDigit, 1);
      break;
    }
    case EDIT_MODE_TENS:
    {
      *editDigit = BCDSub(*editDigit, 0x10);
      break;
    }
  }
  
  if (editMode & EDIT_MODE_ONEBASE)
  {
    if (*editDigit == 0 || *editDigit > editMaxValue)
      *editDigit = 1;
  }
  else
  {
    if (*editDigit > editMaxValue)
    {
      uint8_t diff = BCDSub( 0x99, *editDigit);
      *editDigit = BCDSub(editMaxValue, diff);
    }
  }
} 
