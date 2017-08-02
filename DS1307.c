#include "DateTime.h"
#include "i2c.h"

#define DS1307_ADDR 0xd0

void Read_DS1307_DateTime()
{
  // Read the first 7 registers.

  Read_I2C_Regs(DS1307_ADDR, 0, 7, (uint8_t *)&TheDateTime);
}

void Write_DS1307_DateTime()
{
  Write_I2C_Regs(DS1307_ADDR, 0,7,(uint8_t *)&TheDateTime); // Re-set time and date in same packet to avoid roll-over!
}

// Ensures the DS1307 is correctly configured:
// * Oscillator enabled
// * Enable 24H mode
// * Enable 1 second square wave output

void Init_DS1307()
{
  Read_DS1307_DateTime();

  // See if any of the flags included in the time registers needs changing, as that boils down to
  // re-setting the time.
  if ((TheDateTime.sec & 0x80) || (TheDateTime.hour & 0x40))
  {
    TheDateTime.sec &= ~0x80; // Enable oscillator
    TheDateTime.hour &= ~0x40; // Use 24 hour mode
    Write_DS1307_DateTime();
  }

  // write control register
  uint8_t control = 0x10; // 1Hz clock
  Write_I2C_Regs(DS1307_ADDR, 7,1,&control);
}

