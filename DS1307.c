#include "DateTime.h"
#include "Timefuncs.h"
#include "i2c.h"

#define DS1307_ADDR 0xd0

void Read_DS1307_DateTime()
{
  // Read the first 7 registers.

  Read_I2C_Regs(DS1307_ADDR, 0, 7, (uint8_t *)&TheDateTime);
  UTCToCentralEuropeanTime(&TheDateTime);
}

void Write_DS1307_DateTime()
{
  struct DateTime utcTime = TheDateTime;
  CentralEuropeanTimeToUTC(&utcTime);
  Write_I2C_Regs(DS1307_ADDR, 0,7,(uint8_t *)&utcTime); // Re-set time and date in same packet to avoid roll-over!
}

// Ensures the DS1307 is correctly configured:
// * Oscillator enabled
// * Enable 24H mode
// * Enable 1 second square wave output

void Init_DS1307()
{
  // Read the current time (7 registers). Do not use Read_DS1307_DateTime, as it will apply timezone
  Read_I2C_Regs(DS1307_ADDR, 0, 7, (uint8_t *)&TheDateTime); 
  
  // See if any of the flags included in the time registers needs changing, as that boils down to
  // re-setting the time.
  if ((TheDateTime.sec & 0x80) || (TheDateTime.hour & 0x40))
  {
    TheDateTime.sec &= ~0x80; // Enable oscillator
    TheDateTime.hour &= ~0x40; // Use 24 hour mode
    // Do not use Write_DS1307_DateTime(), as it will de-apply the timezone..
    Write_I2C_Regs(DS1307_ADDR, 0,7,(uint8_t *)&TheDateTime); // Re-set time and date in same packet to avoid roll-over!
  }

  // write control register
  uint8_t control = 0x10; // 1Hz clock
  Write_I2C_Regs(DS1307_ADDR, 7,1,&control);
  
  UTCToCentralEuropeanTime(&TheDateTime);
}

void Read_DS1307_RAM(uint8_t *data, uint8_t addr, uint8_t size)
{
  Read_I2C_Regs(DS1307_ADDR, addr + 8, size, data);
}

void Write_DS1307_RAM(uint8_t *data, uint8_t addr, uint8_t size)
{
  Write_I2C_Regs(DS1307_ADDR, addr + 8, size, data);
}
