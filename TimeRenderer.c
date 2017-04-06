#include "Panels.h"
#include "TimeRenderer.h"
#include "font.h"
#include "7Segment.h"

static uint8_t previousHour = 0, previousMinute = 0, currentHour = 0, currentMinute = 0, currentSecond = 0,
	       animationState = 0;

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
  uint8_t tworow = pgm_read_byte(font + digitToShow * 4 + row/2);

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
  uint8_t data[4]; // Data to be sent to the panels
  
  // Pre-compute the data for the 7-segment displays, it needs to be rotated
  uint8_t segmentDigits[8];
 
  for (uint8_t i = 0; i < 8; ++i)
    segmentDigits[i] = 0; // space
  
  segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + (currentSecond >> 4));
  segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (currentSecond & 0xf));
  for (uint8_t i = 0; i < 8; ++i)
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
    data[2] =  (loMins << 1);
   
    // Rotate the 7-segment data
    const uint8_t row_mask = 1<<i;

    data[3] = 0;

    for(uint8_t j = 0, column_mask = 1; j < 7; ++j, column_mask <<= 1)
      if (segmentDigits[j] & row_mask)
	data[3] |= column_mask;
    
    SendRow(i, data);
  }

}

bool TimeRenderer_Tick()
{
  if (animationState)
  {
    animationState--;
    __privateRender();
  }

  return animationState != 0;
}

bool TimeRenderer_SetTime(uint8_t hour, uint8_t minutes, uint8_t seconds, _Bool animate)
{
  _Bool dotmatrixChanged = (hour != currentHour || minutes != currentMinute);

  if (!dotmatrixChanged && seconds == currentSecond)
    return false;

  previousHour = currentHour;
  previousMinute = currentMinute;

  currentHour = hour;
  currentMinute = minutes;
  currentSecond = seconds;

  if (animate && dotmatrixChanged)
  {
    animationState = 8;
  }
  else
  {
    animationState = 0;
  }
  
  __privateRender();

  return animationState != 0;
}
