#include "Panels.h"
#include "font.h"

static uint8_t previousHour = 0, previousMinute = 0, currentHour = 0, currentMinute = 0, animationState = 0;

static uint8_t __getDigitMask(uint8_t prevDigit, uint8_t currentDigit, int8_t row)
{
  uint8_t digitToShow = currentDigit;
  if (currentDigit != prevDigit)
    row = row - animationState; // use scrolling
 
  if (row == -1)
  {
    return 0; // Seperator line
  }
  else if (row < -1)
  {
    // Use prevDigit
    digitToShow = prevDigit;
    row = row + 8; // wrap around
  }
  // Two rows per byte, lower nybble first.
  uint8_t tworow = font[digitToShow][row/2];

  if (row & 1)
  {
    return tworow >> 4;
  }
  else
  {
    return tworow & 0xf;
  }
}
static void __privateRender()
{
  uint8_t data[3];

  for (int i = 0; i < 8; ++i)
  {
    //hours: tens digit from 0-3, space at 4, ones at 5-8
    const uint8_t 
            hiHours =  __getDigitMask(previousHour >> 4, currentHour >> 4, i),
            loHours =  __getDigitMask(previousHour & 0xf, currentHour & 0xf, i);
  
    data[0] = hiHours | (loHours << 5);  

    // space at 9, dot at 10, space at 11
    const uint8_t 
	    dotMask =  (i == 1 || i == 5) ? 0x04 : 0;

    // mins: tens digit 12-15.
    const uint8_t 
            hiMins  = __getDigitMask(previousMinute >> 4, currentMinute >> 4, i),
            loMins  = __getDigitMask(previousMinute & 0xf, currentMinute & 0xf, i);

    data[1] = (loHours >> 3) | dotMask | (hiMins << 4);
    
    // space at 16, ones digit at 17-21
    data[2] = (panelBitMask[i * 3 + 2] & 0xe0) | (loMins << 1);
    
    SendRow(i, data);
  }

}

void TimeRenderer_Tick()
{
  if (animationState)
  {
    animationState--;
    __privateRender();
  }
}

void TimeRenderer_SetTime(uint8_t hour, uint8_t minutes, _Bool animate)
{
  previousHour = currentHour;
  previousMinute = currentMinute;

  currentHour = hour;
  currentMinute = minutes;

  if (animate)
    animationState = 8;
  else
  {
    animationState = 0;
    __privateRender();
  }
}
