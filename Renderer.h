#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <stdbool.h>
#include <inttypes.h>

#define MAIN_MODE_TIME	    0
#define MAIN_MODE_DATE	    1

#define SECONDARY_MODE_SEC    0
#define SECONDARY_MODE_YEAR   1
#define SECONDARY_MODE_RADIO  2
#define SECONDARY_MODE_VOLUME 3

void Renderer_Tick( const uint8_t secondaryMode ); // Must be called regularly for animations 

// Update contents. if _animate is true, use "rolling" animation.
void Renderer_Update_Main(const uint8_t mainMode,const bool animate);

void Renderer_SetLed(const uint8_t leftLedState, const uint8_t rightLedState);

void Renderer_Update_Secondary();

// Set which digits should be flashing. bit 7 = leftmost digit, bit 0 = rightmost digit
void Renderer_SetFlashMask(const uint8_t mask);
#endif

