#include "Panels.h"
#include "Renderer.h"
#include "font.h"
#include "bitmap.h"
#include "7Segment.h"
#include "DateTime.h"
#include "settings.h"
#include "SI4702.h"

static uint8_t previousHour = 0, previousMinute = 0, animationState = 0, myMainMode = 0, ledState = 0;

static uint16_t blinkMask = 0x0;

static uint8_t blinkStatus = 0;

static const struct AlarmSetting *alarm = 0;

#define BLINK_PERIOD 10
#define BLINK_SHORT_PERIOD  3
#define BLINK_LONG_PERIOD 17

void Renderer_SetAlarmStruct( const struct AlarmSetting *pAlarm )
{
  alarm = pAlarm;
}

static uint8_t __getDigitMaskSingle(const uint8_t character, const int8_t row)
{
  // Two rows per byte, lower nybble first.
  uint8_t tworow = pgm_read_byte(font +  character * 4 + row/2);

  if (row & 1)
  {
    return tworow >> 4;
  }
  else
  {
    return tworow & 0xf;
  }
}

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
  
  return __getDigitMaskSingle(digitToShow, row);
}
static void __privateRender(const uint8_t secondaryMode)
{
  uint8_t data[4]; // Data to be sent to the panels

  // Pre-compute the data for the 7-segment displays, it needs to be rotated
  uint8_t segmentDigits[8] = { 0 };
  uint8_t myLedState = ledState;
  uint8_t thisLedState = myLedState & 0x03;
  myLedState >>= 2;
  
  if (thisLedState == LED_ON || ( thisLedState == LED_BLINK_LONG && (blinkStatus < BLINK_LONG_PERIOD)) || (thisLedState == LED_BLINK_SHORT && (blinkStatus < BLINK_SHORT_PERIOD)))
  {
    segmentDigits[DIGIT_LEFTMOST_LED] = SEG_LED;
  }
  
  thisLedState = myLedState & 0x03;
  myLedState >>= 2;
  
  if (thisLedState == LED_ON || ( thisLedState == LED_BLINK_LONG && (blinkStatus < BLINK_LONG_PERIOD)) || (thisLedState == LED_BLINK_SHORT && (blinkStatus < BLINK_SHORT_PERIOD)))
  {
    segmentDigits[DIGIT_LEFT_LED] = SEG_LED;
  }

  thisLedState = myLedState & 0x03;
  myLedState >>= 2;
  
  if (thisLedState == LED_ON || ( thisLedState == LED_BLINK_LONG && (blinkStatus < BLINK_LONG_PERIOD)) || (thisLedState == LED_BLINK_SHORT && (blinkStatus < BLINK_SHORT_PERIOD)))
  {
    segmentDigits[DIGIT_RIGHT_LED] = SEG_LED;
  }
  
  thisLedState = myLedState & 0x03;
  
  if (thisLedState == LED_ON || ( thisLedState == LED_BLINK_LONG && (blinkStatus < BLINK_LONG_PERIOD)) || (thisLedState == LED_BLINK_SHORT && (blinkStatus < BLINK_SHORT_PERIOD)))
  {
    segmentDigits[DIGIT_RIGHTMOST_LED] = SEG_LED;
  }
  
  switch (secondaryMode)
  {
    case SECONDARY_MODE_SEC:
      segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + (TheDateTime.sec >> 4));
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (TheDateTime.sec & 0xf));
      
      // (ab)use the decimal dot of digit 4 to indicate whether DST is active
      segmentDigits[DIGIT_4] = (TheGlobalSettings.dstActive ? SEG_DP : 0);
      break;
    case SECONDARY_MODE_YEAR:
      segmentDigits[DIGIT_1] = pgm_read_byte(BCDToSegment + 2);
      segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + 0);
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (TheDateTime.year >> 4));
      segmentDigits[DIGIT_4] = pgm_read_byte(BCDToSegment + (TheDateTime.year & 0xf));
      break;
    
    case SECONDARY_MODE_VOLUME:
    {
      uint8_t vol = SI4702_GetVolume();
      segmentDigits[DIGIT_4] = SEG_d;
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (vol%10));
      segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + (vol/10));
      segmentDigits[DIGIT_1] = SEG_d;
      break;
    }
    case SECONDARY_MODE_ALARM:
    {
      if (!alarm || !(alarm->flags & ALARM_TYPE_RADIO))
      {
	segmentDigits[DIGIT_1] = SEG_f | SEG_g | SEG_e | SEG_c | SEG_d; // lowercase b
	segmentDigits[DIGIT_2] = SEG_a | SEG_f | SEG_g | SEG_e | SEG_d; // Uppercase E
	segmentDigits[DIGIT_3] = SEG_a | SEG_f | SEG_g | SEG_e | SEG_d; // Uppercase E
	segmentDigits[DIGIT_4] = SEG_a | SEG_f | SEG_b | SEG_g | SEG_e; // lowecase p
	break;
      }
      // fall-through
    }
    case SECONDARY_MODE_RADIO:
    {
      uint16_t freq;
      freq = TheGlobalSettings.radio.frequency;

      segmentDigits[DIGIT_4] = pgm_read_byte(BCDToSegment + (freq%10));
      freq /= 10;
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + (freq%10)) | SEG_DP;
      freq /= 10;
      segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + (freq%10));
      freq /= 10;
      if(freq)
	segmentDigits[DIGIT_1] = pgm_read_byte(BCDToSegment + (freq%10));

      break;
    }
    case SECONDARY_MODE_NAP:
    {
      uint8_t hours = TheNapTime / 60;
      uint8_t mins = TheNapTime % 60;
      segmentDigits[DIGIT_4] = pgm_read_byte(BCDToSegment + (mins%10));
      mins /= 10;
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + mins);
      
      if (hours)
      {
	segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + hours) | SEG_DP;
      }
      break;
    }
    case SECONDARY_MODE_SLEEP:
    {
      uint8_t hours = TheSleepTime / 60;
      uint8_t mins = TheSleepTime % 60;
      segmentDigits[DIGIT_4] = pgm_read_byte(BCDToSegment + (mins%10));
      mins /= 10;
      segmentDigits[DIGIT_3] = pgm_read_byte(BCDToSegment + mins);
      
      if (hours)
      {
	segmentDigits[DIGIT_2] = pgm_read_byte(BCDToSegment + hours) | SEG_DP;
      }
      break;
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
  
  
  for (uint8_t i = 0; i < 8; ++i)
  {
    if (myMainMode < MAIN_MODE_SLEEP)
    {
      uint8_t mainDigit[4] = {0};
      uint8_t dotMask = 0; 
      uint8_t wday_mask = 0;
      switch(myMainMode)
      {
	case MAIN_MODE_TIME:
	  mainDigit[0] = __getDigitMask(previousHour >> 4, TheDateTime.hour >> 4, i);
	  mainDigit[1] = __getDigitMask(previousHour &0xf, TheDateTime.hour & 0xf, i);
	  mainDigit[2] = __getDigitMask(previousMinute >> 4, TheDateTime.min >> 4, i);
	  mainDigit[3] = __getDigitMask(previousMinute &0xf, TheDateTime.min & 0xf, i);
	  dotMask =  (i == 1 || i == 5) ? 0x20 : 0;
	  wday_mask =  ( i == TheDateTime.wday - 1) ? 0x3: 0;
	  break;
	case MAIN_MODE_DATE:
	  mainDigit[0] = __getDigitMaskSingle(TheDateTime.day >> 4, i);
	  mainDigit[1] = __getDigitMaskSingle(TheDateTime.day &0xf, i);
	  mainDigit[2] = __getDigitMaskSingle(TheDateTime.month >> 4, i);
	  mainDigit[3] = __getDigitMaskSingle(TheDateTime.month &0xf, i);
	  dotMask = (i < 2 ? 0x40 : (i <5 ? 0x20 : (i < 7 ? 0x10 : 0)));
	  wday_mask =  ( i == TheDateTime.wday - 1) ? 0x3: 0;
	  break;
	case MAIN_MODE_ALARM:
	  if (alarm)
	  {
	    mainDigit[0] = __getDigitMaskSingle(alarm->hour >>4, i);
	    mainDigit[1] = __getDigitMaskSingle(alarm->hour &0xf, i);
	    mainDigit[2] = __getDigitMaskSingle(alarm->min >> 4, i);
	    mainDigit[3] = __getDigitMaskSingle(alarm->min &0xf, i);
	    dotMask =  (i == 1 || i == 5) ? 0x20 : 0;
	    if (i == 7)
	      wday_mask = 0;
	    else
	    {
	      switch (alarm->flags & ALARM_DAY_BITS)
	      {
		case ALARM_DAY_DAILY:
		  wday_mask = 3;
		  break;
		case ALARM_DAY_WEEK:
		  wday_mask = (i < 5) ? 3 : 0;
		  break;
		case ALARM_DAY_WEEKEND:
		  wday_mask = (i >= 5 && i ) ? 3 : 0;
		  break;
	      }
	    }
	  }
	  break;
      }
      
      if (blinkStatus < BLINK_PERIOD)
      {
	uint8_t digitMask = blinkMask & 0xff;
	for (uint8_t j = 0, mask = 0x80; j < 4; ++j, mask >>= 1)
	{
	  if (digitMask & mask)
	    mainDigit[j] = ( i == 7 ? 0x0f : 0);
	}
	
	if (blinkMask & 0x100)
	  wday_mask = (i == 7 ? 3 : 0);
      }
      
      // day-of-week on 0 and 1, Digit 0 at 3, space at 7
      data[0] = (mainDigit[0] <<3) | wday_mask ;

      // Digit 1 at 8-10, space at 11, dot at 12, space at 13, digit 2 at 14-17

      data[1] = (mainDigit[1] | dotMask | (mainDigit[2] << 7));
      
      // space at 18, digit 3 at 19-23
      data[2] = (mainDigit[2] >> 1) | (mainDigit[3] << 4);
    }
    else if (myMainMode == MAIN_MODE_SLEEP)
    {
      // Write out "SLEEP" bitmap
      const uint8_t *b = Bitmap_Sleep + (i * 3);
      data[0] = pgm_read_byte(b + 0);
      data[1] = pgm_read_byte(b + 1);
      data[2] = pgm_read_byte(b + 2);
    }     
    else if (myMainMode == MAIN_MODE_NAP)
    {
      // Write out "NAP" bitmap
      const uint8_t *b = Bitmap_Nap + (i * 3);
      data[0] = pgm_read_byte(b + 0);
      data[1] = pgm_read_byte(b + 1);
      data[2] = pgm_read_byte(b + 2);
    }
    // Rotate the 7-segment data
    const uint8_t row_mask = 1<<i;

    data[3] = 0;

    for(uint8_t j = 0, column_mask = 1; j < 8; ++j, column_mask <<= 1)
      if (segmentDigits[j] & row_mask)
	data[3] |= column_mask;
    
    SendRow(i, data);
  }

}

void Renderer_Tick(uint8_t secondaryMode)
{
  blinkStatus = (blinkStatus + 1) % ( 2 * BLINK_PERIOD);
  
  _Bool doRender = 0;
  
  uint8_t ledBlinking = ledState & 0xaa; // Blink on 10 and 11, not on 00 and 01
  
  if (animationState & 0x80 || (blinkMask && (blinkStatus == 0 || blinkStatus == BLINK_PERIOD)) || ledBlinking != 0)
  {
    animationState &= 0x7f;
    doRender = 1;
  }
  
  if (animationState)
  {
    animationState--;
    
    doRender = 1;
    
    if (animationState == 0)
    {
      previousHour = TheDateTime.hour;
      previousMinute = TheDateTime.min;
    }
  } 
  
  if (doRender)
    __privateRender(secondaryMode);
}

void Renderer_Update_Secondary()
{
  animationState |= 0x80;
}

void Renderer_SetLed(const uint8_t leftMostLedState, const uint8_t leftLedState, const uint8_t rightLedState, const uint8_t rightMostLedState)
{
  ledState = rightMostLedState;
  ledState <<= 2;
  ledState |= rightLedState;
  ledState <<= 2;
  ledState |= leftLedState;
  ledState <<= 2;
  ledState |= leftMostLedState;
  
  Renderer_Update_Secondary();
}

void Renderer_Update_Main(const uint8_t mainMode,const _Bool animate)
{
  _Bool dotmatrixChanged = (mainMode == MAIN_MODE_TIME && (previousHour != TheDateTime.hour || previousMinute != TheDateTime.min));

  myMainMode = mainMode;
  
  if (animate && dotmatrixChanged)
  {
    animationState = 9;
  }
  else
  {
    animationState = 1;
  }
}

void Renderer_SetFlashMask(const uint16_t mask)
{
  blinkMask = mask; 
}
