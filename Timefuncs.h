#ifndef __TIMEFUNCS_H__
#define __TIMEFUNCS_H__
#include <inttypes.h>

struct DateTime;

uint8_t GetDaysPerMonth(const uint8_t Month, const uint8_t year /* 20XX */); 
uint8_t GetDayOfWeek(uint8_t Day, uint8_t Month, uint8_t year /* 20xx */);
uint8_t GetDateOfLastSunday(uint8_t month, uint8_t year) ;

_Bool ItIsDarkOutside(const struct DateTime *timestamp);

// Exposed for test
void NormalizeHours(struct DateTime *timestamp);
_Bool IsDSTActive(const struct DateTime *timestamp, _Bool timestampIsUTC);

void CentralEuropeanTimeToUTC(struct DateTime *TheDateTime);
void UTCToCentralEuropeanTime(struct DateTime *TheDateTime);
#endif
