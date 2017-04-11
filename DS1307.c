#include <avr/io.h>
#include "DateTime.h"

inline void I2CWait()
{
  while (!(TWCR & _BV(TWINT)))
    ; // Wait
}

void Write_DS1307_regs(uint8_t reg, uint8_t amount, uint8_t *ptr)
{
  if (amount == 0)
    return;

  // Send START
  TWCR = _BV(TWINT)| _BV(TWSTA) | _BV(TWEN);

  I2CWait();

  if ((TWSR & 0xF8) != 0x08)
    return;

  TWDR=0xd0;  // Slave address, write
  TWCR = _BV(TWEN) | _BV(TWINT);
  I2CWait();

  if ((TWSR & 0xf8) != 0x18)
    goto error; // Not acked

  // Send register address

  TWDR = reg;
  TWCR = _BV(TWEN) | _BV(TWINT);
  I2CWait();

  if ((TWSR & 0xf8) != 0x28)
    goto error; // Not ack.

  while(amount--)
  {
    TWDR = *ptr++;
    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA); // Clear TWINT to resume,
    I2CWait();
    if((TWSR & 0xf8) != 0x28)
      break;
  }

error:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); // Stop    
  // Wait on STOP to clear
  while ((TWCR & _BV(TWSTO)))
    ;
}
void Read_DS1307_regs(uint8_t reg, uint8_t amount, uint8_t *ptr)
{
  if (amount == 0)
    return;

  // Send START
  TWCR = _BV(TWINT)| _BV(TWSTA) | _BV(TWEN);

  I2CWait();

  if ((TWSR & 0xF8) != 0x08)
    return;

  TWDR=0xd0;  // Slave address, write
  TWCR = _BV(TWEN) | _BV(TWINT);
  I2CWait();

  if ((TWSR & 0xf8) != 0x18)
    goto error; // Not acked

  // Send register address

  TWDR = reg;
  TWCR = _BV(TWEN) | _BV(TWINT);
  I2CWait();

  if ((TWSR & 0xf8) != 0x28)
    goto error; // Not ack.

  // Send repeated start
  TWCR = _BV(TWINT)| _BV(TWSTA) | _BV(TWEN);
  I2CWait();

  if ((TWSR & 0xF8) != 0x10)
    goto error;

  TWDR=0xd1;  // Slave address, read
  TWCR = _BV(TWEN) | _BV(TWINT);
  I2CWait();

  if ((TWSR & 0xf8) != 0x40)
    goto error; // Not acked

  while(--amount)
  {
    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA); // Clear TWINT to resume, send ack to indicate more data is requested
    I2CWait();
    if((TWSR & 0xf8) != 0x50)
      goto error; // Not acked.

    *ptr++ = TWDR;
  }

  //read last byte

  TWCR = _BV(TWEN) | _BV(TWINT); // Clear TWINT to resume, send nack 
  I2CWait();
  *ptr++ = TWDR;

error:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); // Stop    
  // Wait on STOP to clear
  while ((TWCR & _BV(TWSTO)))
    ;

}

void Read_DS1307_DateTime()
{
  // Read the first 7 registers.

  Read_DS1307_regs(0, 7, (uint8_t *)&TheDateTime);
}

void Write_DS1307_DateTime()
{
  Write_DS1307_regs(0,7,(uint8_t *)&TheDateTime); // Re-set time and date in same packet to avoid roll-over!
}

// Ensures the DS1307 is correctly configured:
// * Oscillator enabled
// * Enable 24H mode
// * Enable 1 second square wave output

void Init_DS1307()
{
  // Set up I2C
  
  DDRC = 0; // entire port input
  PORTC = 0x3f; // Enable pull-up
  DDRC = 0x30; // PC4 and PC5 to output.

  // Configure I2C speed: 100 KHz @ 16MHz clock
  TWSR = 0;
  TWBR = 72;
  TWCR = _BV(TWEN); // enable I2C
  
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
  Write_DS1307_regs(7,1,&control);
}

