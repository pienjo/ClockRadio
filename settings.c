#include "settings.h"
#include "DS1307.h"
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
  
  return checksum == CalculateCRC();
}

void WriteGlobalSettings()
{
  uint8_t checksum = CalculateCRC();
  Write_DS1307_RAM(&checksum, 0, 1);
  Write_DS1307_RAM((uint8_t *) &TheGlobalSettings, 1, sizeof(struct GlobalSettings));
}
