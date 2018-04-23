#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <inttypes.h>
#include "DateTime.h"

struct AlarmSetting
{
  uint8_t hour;
  uint8_t min; 
  // Bitfield:
  // bit 0: Alarm active
  // bit 1: Indicates beeper (0) or radio (1)
  // bit 2-3: Repeat type:
  //        00 : Repeat daily (mon-sun)
  //        01 : Repeat on weekdays
  //        10 : Repeat on weekend
  //        11 : Do not repeat. ( used for temporary alarm only)
  // bit 4: Next invocation of alarm is suspended
  
  #define ALARM_SUSPENDED 0x10
  #define ALARM_DAY_DAILY 0
  #define ALARM_DAY_WEEK  4
  #define ALARM_DAY_WEEKEND 8
  #define ALARM_DAY_NEVER 0xc
  #define ALARM_DAY_BITS  0xc
  #define ALARM_TYPE_RADIO 2
  #define ALARM_ACTIVE     1
  uint8_t flags;
};

struct RadioSettings
{
  uint16_t frequency;
  uint8_t  volume;
};


struct GlobalSettings
{
  struct RadioSettings radio;
  uint8_t              brightness;
  uint8_t              brightness_night;
  struct AlarmSetting  alarm1, 
                       alarm2,
		       onetime_alarm;
};

extern struct GlobalSettings TheGlobalSettings;

_Bool ReadGlobalSettings(); // returns true if settings were sucessfully read.
void WriteGlobalSettings();

uint8_t GetActiveBrightness(const struct DateTime *timestamp);
uint8_t IncreaseBrightness(const struct DateTime *timestamp);
uint8_t DecreaseBrightness(const struct DateTime *timestamp);
#endif
