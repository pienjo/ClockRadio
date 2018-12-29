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
#include "longpress.h"
#include "events.h"

#define LONGPRESS_TICKS 21
#define REP_TICKS 5

static uint8_t state[8];
  
void GetLongPress(const uint16_t events, struct longPressResult *result)
{
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
	result->shortPress |= mask;
	
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
	  result->longPress |= mask;
	  result->repPress |= mask;
	} else if (state[idx] == LONGPRESS_TICKS + REP_TICKS)
	{
	  result->repPress |= mask;
	  state[idx] = LONGPRESS_TICKS;
	}
      }
    } 
  }
  
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
