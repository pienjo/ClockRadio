#include "longpress.h"
#include "events.h"

#define LONGPRESS_TICKS 21
#define REP_TICKS 5

static uint8_t state[8];
  
struct longPressResult GetLongPress(const uint16_t events)
{
  struct longPressResult r;
  
  r.shortPress = 0;
  r.longPress = 0;
  r.repPress = 0;
  
  // see if released
  
  const uint8_t releaseEvents = (events >> 8);
  const uint8_t pressEvents = events & 0xff;
  
  for (uint8_t idx = 0, mask = 1; idx < 7; ++idx, mask = mask << 1)
  {
    if (pressEvents & mask)
    {
      state[idx] = 1;
    }  
  
    if (releaseEvents & mask)
    {
      if (state[idx] < LONGPRESS_TICKS)
	r.shortPress |= mask;
	
      state[idx] = 0;
    }  
  }
  
  if (events & CLOCK_TICK)
  {
    // see if threshold met
    
    for (uint8_t idx = 0, mask = 1; idx < 7; ++idx, mask = mask << 1)
    {
      if (state[idx]) 
      {
	state[idx]++;
	if (state[idx] == LONGPRESS_TICKS)
	{
	  r.longPress |= mask;
	  r.repPress |= mask;
	} else if (state[idx] == LONGPRESS_TICKS + REP_TICKS)
	{
	  r.repPress |= mask;
	  state[idx] = LONGPRESS_TICKS;
	}
      }
    } 
  }
  
  return r;
}

void MarkLongPressHandled(const uint8_t handledMask)
{
  for (uint8_t idx = 0, mask = 1; idx < 7; ++idx, mask = mask << 1)
  {
    if (handledMask & mask)
    {
      state[idx] = LONGPRESS_TICKS;
    }
  }
}
