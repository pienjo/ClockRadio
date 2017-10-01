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
