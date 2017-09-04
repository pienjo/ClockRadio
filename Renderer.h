#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <stdbool.h>
#include <inttypes.h>

enum enumMainMode
{
  MAIN_MODE_TIME,
  MAIN_MODE_DATE,
  MAIN_MODE_ALARM1,
  MAIN_MODE_ALARM2
};

enum enumSecondaryMode
{
  SECONDARY_MODE_SEC,
  SECONDARY_MODE_YEAR,
  SECONDARY_MODE_RADIO,
  SECONDARY_MODE_VOLUME,
  SECONDARY_MODE_ALARM1,
  SECONDARY_MODE_ALARM2,
};

void Renderer_Tick( const uint8_t secondaryMode ); // Must be called regularly for animations 

// Update contents. if _animate is true, use "rolling" animation.
void Renderer_Update_Main(const uint8_t mainMode,const bool animate);

enum enumLedMode
{
  LED_OFF,
  LED_BLINK_SHORT,
  LED_BLINK_LONG,
  LED_ON
};

void Renderer_SetLed(const uint8_t leftLedState, const uint8_t rightLedState);

void Renderer_Update_Secondary();

// Set which digits should be flashing. bit 7 = leftmost digit, bit 0 = rightmost digit
void Renderer_SetFlashMask(const uint8_t mask);
#endif

