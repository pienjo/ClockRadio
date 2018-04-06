#include "Timefuncs.h"
#include "DateTime.h"
#include <avr/pgmspace.h>
#include <stdbool.h>

const uint8_t PROGMEM dpm[] =
{
  0x31, // jan
  0x28, // feb
  0x31, // mar
  0x30, // apr
  0x31, // may
  0x30, // jun
  0x31, // jul
  0x31, // aug
  0x30, // sep
  0x31, // oct
  0x30, // nov
  0x31, // dec;
};

uint8_t GetDaysPerMonth(const uint8_t Month, const uint8_t Year)
{
  uint8_t days = pgm_read_byte(dpm + Month-1);
  if (Month == 2 && (Year %4 == 0))
    days++; // February on a leap year
  
  return days;
}

const uint8_t PROGMEM dowTable[] = { 0,3,2,5,0,3,5,1,4,6,2,4 };

uint8_t GetDayOfWeek(uint8_t Day, uint8_t Month, uint8_t year /* 20xx */) // 1 - monday
{
  year = year >> 4 | (year & 0xf); // Undo BCD
  year -= ( Month < 3);
  
  return  ((year + year / 4 /* -y/100 + y / 400 */ + pgm_read_byte(dowTable + Month -1) + Day) % 7) + 1;
}

static void NormalizeHours(struct DateTime *timestamp)
{
  if (timestamp->hour >= 0 && timestamp->hour < 24)
    return; // Nothing to do.
  
  while((int8_t) timestamp->hour < 0)
  {
    timestamp->day--;
    timestamp->wday = (timestamp->wday - 1) % 7;
    timestamp->hour += 24;
  }  
  
  while(timestamp->hour >= 24)
  {
    timestamp->day--;
    timestamp->wday = (timestamp->wday + 1) % 7;
    timestamp->hour -= 24;
  }
  
  if ((int8_t) timestamp->day < 1)
  {
    timestamp->month--;
    if (timestamp->month == 0)
    {
      timestamp->year--;
      timestamp->month = 12;
    }
    
    timestamp->day +=  GetDaysPerMonth(timestamp->month, timestamp->year);
    return;
  }
  
  uint8_t daysThisMonth = GetDaysPerMonth(timestamp->month, timestamp->year);
 
  if (daysThisMonth < timestamp->day)
  {
    timestamp->month++;
    timestamp->day -= daysThisMonth;
    if (timestamp->month == 13)
    {
      timestamp->year ++;
      timestamp->month = 1;
    }
  }
}


static uint8_t GetDateOfLastSunday(uint8_t month, uint8_t year) 
{
  const uint8_t lastDayOfMonth = GetDaysPerMonth(month, year);
  // Determine the day-of-week of that date. If it is a sunday, then that's the answer.
  
  const uint8_t weekdayOfLastDay = GetDayOfWeek(lastDayOfMonth, month, year);
  if (weekdayOfLastDay == 7)
  {
    return lastDayOfMonth;
  }
  
  // If it is monday(1), then the transition is on the previous day. If it was on tuesday(2), it was two days before, etc.
  return lastDayOfMonth - weekdayOfLastDay;
}

static _Bool IsDSTActive(const struct DateTime *timestamp, _Bool timestampIsUTC)
{
  // DST is active from the last sunday of march until the last sunday of october. 
  if (timestamp->month < 3 || timestamp->month > 10) // jan, feb, nov, dec
  {
    return false;
  } else if (timestamp->month > 3 && timestamp->month < 10) // apr, may, jun, jul, aug, sept
  {
    return true;
  }
  
  // march or october. Transition day is the last sunday of this month.
  const uint8_t transitionDay = GetDateOfLastSunday(timestamp->month, timestamp->year);
  _Bool pastTransitionDay;
  if (timestamp->day == transitionDay)
  {
    // Transition from CET to CEST is at 02:00 UTC+1, or at 1:00 UTC.
    // Transition from CEST to CET is at 03:00 UTC+2, or at 1:00 UTC.
    if (timestampIsUTC)
      pastTransitionDay = (timestamp->hour > 0);
    else
    {
      // 02:00 - 03:00 occurs twice in october. 
      // For the sake of simplicity, assume DST is no longer in effect when time is set after 02:00
      pastTransitionDay = (timestamp->hour >= 2); 
    }
  }
  else
  {
    pastTransitionDay = (timestamp->day > transitionDay);
  }
  
  return (timestamp->month == 3 ? pastTransitionDay : !pastTransitionDay);
}
void UTCToCentralEuropeanTime(struct DateTime *timestamp)
{
  if (IsDSTActive(timestamp, true))
    timestamp->hour += 2;
  else
    timestamp->hour += 1;
    
  NormalizeHours(timestamp);
}

void CentralEuropeanTimeToUTC(struct DateTime *timestamp)
{
  if (IsDSTActive(timestamp, false))
    timestamp->hour -= 2;
  else
    timestamp->hour -= 1;
    
  NormalizeHours(timestamp);
}
