#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <stdbool.h>
#include <inttypes.h>

#define MAIN_MODE_TIME	    0
#define MAIN_MODE_DATE	    1

#define SECONDARY_MODE_SEC   0
#define SECONDARY_MODE_YEAR  1
#define SECONDARY_MODE_RADIO 2

void Renderer_Tick(); // Must be called regularly for animations 

// Update contents. if _animate is true, use "rolling" animation.
void Renderer_Update(uint8_t mainMode, uint8_t secondaryMode, bool animate);
#endif

