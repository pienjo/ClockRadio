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
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <stdbool.h>
#include <inttypes.h>
#include <settings.h>

enum enumMainMode
{
  MAIN_MODE_TIME,
  MAIN_MODE_DATE,
  MAIN_MODE_ALARM,
  MAIN_MODE_SLEEP,
  MAIN_MODE_NAP,
};

enum enumSecondaryMode
{
  SECONDARY_MODE_SEC,
  SECONDARY_MODE_YEAR,
  SECONDARY_MODE_RADIO,
  SECONDARY_MODE_VOLUME,
  SECONDARY_MODE_ALARM,
  SECONDARY_MODE_SLEEP,
  SECONDARY_MODE_NAP,
  SECONDARY_MODE_TIME_ADJUST,
};

void Renderer_Init();
void Renderer_Tick( const uint8_t secondaryMode ); // Must be called regularly for animations 

// Update contents. if _animate is true, use "rolling" animation.
void Renderer_Update_Main(const uint8_t mainMode,const bool animate);
void Renderer_SetAlarmStruct( const struct AlarmSetting *alarm );
enum enumLedMode
{
  LED_OFF = 0,
  LED_ON  = 1,
  LED_BLINK_SHORT = 2,
  LED_BLINK_LONG = 3
};

void Renderer_SetLed(const uint8_t leftMostLedState, const uint8_t leftLedState, const uint8_t rightLedState, const uint8_t rightMostLedState);

void Renderer_Update_Secondary();

// Set which digits should be flashing. bit 7 = leftmost digit, bit 0 = rightmost digit. bit 8 = day indicator
void Renderer_SetFlashMask(const uint16_t mask);

enum enumInvertionMode
{
  NOT_INVERTED = 0,
  INVERTED = 1,
};

void Renderer_SetInverted(const enum enumInvertionMode invertionMode);

#endif

