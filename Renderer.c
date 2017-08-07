#include "Panels.h"
#include "Renderer.h"
#include "font.h"
#include "7Segment.h"
#include "DateTime.h"
#include "SI4702.h"

static uint8_t previousHour = 0, previousMinute = 0, animationState = 0, myMainMode = 0, mySecondaryMode = 0;

static uint8_t blinkMask = 0x0;

static uint8_t blinkStatus = 0;

#define BLINK_PERIOD 10

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
  uint8_t segmentDigits[8] = { 0 };
 
  switch (mySecondaryMode)
  {
    case SECONDARY_MODE_SEC:
      segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + (TheDateTime.sec >> 4));
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (TheDateTime.sec & 0xf));
      break;
    case SECONDARY_MODE_YEAR:
      segmentDigits[DIGIT_1] = pgm_read_byte(BCDToSegment + 2);
      segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + 0);
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (TheDateTime.year >> 4));
      segmentDigits[DIGIT_4] = pgm_read_byte(BCDToSegment + (TheDateTime.year & 0xf));
      break;
    case SECONDARY_MODE_RADIO:
    {
      uint16_t freq = SI4702_GetFrequency();

      if (freq == 0)
      {
	// Radio disabled
	segmentDigits[DIGIT_1] = SEG_g;
	segmentDigits[DIGIT_2] = SEG_g;
	segmentDigits[DIGIT_3] = SEG_g;
	segmentDigits[DIGIT_4] = SEG_g;
      }
      else
      {
	segmentDigits[DIGIT_4] = pgm_read_byte(BCDToSegment + (freq%10));
	freq /= 10;
	segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (freq%10)) | SEG_DP;
	freq /= 10;
	segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + (freq%10));
	freq /= 10;
	if(freq)
	  segmentDigits[DIGIT_1] = pgm_read_byte(BCDToSegment + (freq%10));
      }
    }
  }

  if (blinkStatus < BLINK_PERIOD)
  {
    if (blinkMask & 0x08)
      segmentDigits[DIGIT_1] = 0;
    if (blinkMask & 0x04)
      segmentDigits[DIGIT_2] = 0;
    if (blinkMask & 0x02)
      segmentDigits[DIGIT_3] = 0;
    if (blinkMask & 0x01)
      segmentDigits[DIGIT_4] = 0;
  }

  uint8_t mainDigit[4] = {0};
  for (uint8_t i = 0; i < 8; ++i)
  {
    uint8_t dotMask = 0; 
    switch(myMainMode)
    {
      case MAIN_MODE_TIME:
	mainDigit[0] = __getDigitMask(previousHour >> 4, TheDateTime.hour >> 4, i);
	mainDigit[1] = __getDigitMask(previousHour &0xf, TheDateTime.hour & 0xf, i);
	mainDigit[2] = __getDigitMask(previousMinute >> 4, TheDateTime.min >> 4, i);
	mainDigit[3] = __getDigitMask(previousMinute &0xf, TheDateTime.min & 0xf, i);
	dotMask =  (i == 1 || i == 5) ? 0x20 : 0;
	break;
      case MAIN_MODE_DATE:
	mainDigit[0] = __getDigitMask(TheDateTime.day >> 4, TheDateTime.day >> 4, i);
	mainDigit[1] = __getDigitMask(TheDateTime.day &0xf, TheDateTime.day & 0xf, i);
	mainDigit[2] = __getDigitMask(TheDateTime.month >> 4, TheDateTime.month >> 4, i);
	mainDigit[3] = __getDigitMask(TheDateTime.month &0xf, TheDateTime.month & 0xf, i);
	dotMask = (i < 2 ? 0x40 : (i <5 ? 0x20 : (i < 7 ? 0x10 : 0)));
	break;
    }
    
    if (blinkStatus < BLINK_PERIOD)
    {
      for (uint8_t i = 0, mask = 0x80; i < 4; ++i, mask >>= 1)
      {
	if (blinkMask & mask)
	  mainDigit[i] = 0;
      }
    }

    // day-of-week on 0 and 1, Digit 0 at 3, space at 7
    data[0] = (mainDigit[0] <<3) | (( i == TheDateTime.wday - 1) ? 0x3: 0 );

    // Digit 1 at 8-10, space at 11, dot at 12, space at 13, digit 2 at 14-17

    data[1] = (mainDigit[1] | dotMask | (mainDigit[2] << 7));
    
    // space at 18, digit 3 at 19-23
    data[2] = (mainDigit[2] >> 1) | (mainDigit[3] << 4);
   
    // Rotate the 7-segment data
    const uint8_t row_mask = 1<<i;

    data[3] = 0;

    for(uint8_t j = 0, column_mask = 1; j < 7; ++j, column_mask <<= 1)
      if (segmentDigits[j] & row_mask)
	data[3] |= column_mask;
    
    SendRow(i, data);
  }

}

void Renderer_Tick()
{
  blinkStatus = (blinkStatus + 1) % ( 2 * BLINK_PERIOD);
  
  if (animationState)
  {
    animationState--;
    __privateRender();
    if (animationState == 0)
    {
      previousHour = TheDateTime.hour;
      previousMinute = TheDateTime.min;
    }
  } else if (blinkMask && (blinkStatus == 0 || blinkStatus == BLINK_PERIOD))
    __privateRender();
}

void Renderer_Update(uint8_t mainMode, uint8_t secondaryMode, _Bool animate)
{
  _Bool dotmatrixChanged = (mainMode == MAIN_MODE_TIME && (previousHour != TheDateTime.hour || previousMinute != TheDateTime.min));

  myMainMode = mainMode;
  mySecondaryMode = secondaryMode;

  if (animate && dotmatrixChanged)
  {
    animationState = 9;
  }
  else
  {
    blinkStatus = BLINK_PERIOD;
    animationState = 1;
  }
}

void Renderer_SetFlashMask(uint8_t mask)
{
  blinkMask = mask; 
  blinkStatus = BLINK_PERIOD - 2;
}
