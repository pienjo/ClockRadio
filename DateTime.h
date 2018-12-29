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
#ifndef __DATETIME_H__
#define __DATETIME_H__
#include <inttypes.h>

struct DateTime
{
  // All in BCD.

  uint8_t sec;
  uint8_t min; 
  uint8_t hour;
  uint8_t wday; // weekday, 1-7
  uint8_t day;
  uint8_t month;
  uint8_t year; // 20xx
};

extern struct DateTime TheDateTime;
extern uint8_t TheSleepTime; // In minutes
extern uint8_t TheNapTime; // In minutes

#endif
