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
#include "settings.h"
#include "DS1307.h"
#include "Timefuncs.h"
#include <util/crc16.h>

struct GlobalSettings TheGlobalSettings;

static uint8_t CalculateCRC()
{
  const uint8_t *data = (const uint8_t *) &TheGlobalSettings;
  uint8_t crc = 0;
  
  for (uint8_t i = 0; i < sizeof(struct GlobalSettings); ++i)
    crc = _crc8_ccitt_update(crc, *data++);

  return crc;
}

// returns true if settings were sucessfully read.
_Bool ReadGlobalSettings()
{
  // Read checksum
  uint8_t checksum;
  Read_DS1307_RAM(&checksum, 0, 1);
  
  Read_DS1307_RAM((uint8_t *) &TheGlobalSettings, 1, sizeof(struct GlobalSettings));
  
  _Bool checksumOK = (checksum == CalculateCRC());
  
  // Remove any suspend flags
  TheGlobalSettings.alarm1.flags &= ~(ALARM_SUSPENDED);
  TheGlobalSettings.alarm2.flags &= ~(ALARM_SUSPENDED);
  
  return checksumOK;
}

void WriteGlobalSettings()
{
  uint8_t checksum = CalculateCRC();
  Write_DS1307_RAM(&checksum, 0, 1);
  Write_DS1307_RAM((uint8_t *) &TheGlobalSettings, 1, sizeof(struct GlobalSettings));
}

uint8_t GetActiveBrightness(const struct DateTime *timestamp)
{
  if (ItIsDarkOutside(timestamp))
    return TheGlobalSettings.brightness_night;
  else
    return TheGlobalSettings.brightness;
}

uint8_t IncreaseBrightness(const struct DateTime *timestamp)
{
  if (ItIsDarkOutside(timestamp))
  {
    if (TheGlobalSettings.brightness_night < 15)
      TheGlobalSettings.brightness_night++;
    return TheGlobalSettings.brightness_night;
  }
  else
  {
    if (TheGlobalSettings.brightness < 15)
      TheGlobalSettings.brightness++;
    return TheGlobalSettings.brightness;
  }
}

uint8_t DecreaseBrightness(const struct DateTime *timestamp)
{
  if (ItIsDarkOutside(timestamp))
  {
    if (TheGlobalSettings.brightness_night != 0)
      TheGlobalSettings.brightness_night--;
    return TheGlobalSettings.brightness_night;
  }
  else
  {
    if (TheGlobalSettings.brightness != 0)
      TheGlobalSettings.brightness--;
    return TheGlobalSettings.brightness;
  }
}
