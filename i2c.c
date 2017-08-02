#include "i2c.h"
#include <avr/io.h>

inline void I2CWait()
{
  while (!(TWCR & _BV(TWINT)))
    ; // Wait
}

void Write_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, uint8_t *ptr)
{
  if (amount == 0)
    return;

  // Send START
  TWCR = _BV(TWINT)| _BV(TWSTA) | _BV(TWEN);

  I2CWait();

  if ((TWSR & 0xF8) != 0x08)
    return;

  TWDR= addr & 0xfe;  // Slave address, write

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

void Read_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, uint8_t *ptr)
{
  if (amount == 0)
    return;

  // Send START
  TWCR = _BV(TWINT)| _BV(TWSTA) | _BV(TWEN);

  I2CWait();

  if ((TWSR & 0xF8) != 0x08)
    return;

  TWDR=addr & 0xfe;  // Slave address, write
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

  TWDR=addr | 0x01;  // Slave address, read
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

void Init_I2C()
{
  // Set up I2C
  DDRC &= ~(_BV(PORTC4));
  DDRC &= ~(_BV(PORTC5));

  PORTC |= _BV(PORTC4);
  PORTC |= _BV(PORTC5); // Enable pull-up on I2C pins
  DDRC |= _BV(PORTC4) | _BV(PORTC5); // PC4 and PC5 to output.
  
  // Configure I2C speed: 100 KHz @ 16MHz clock
  TWSR = 0;
  TWBR = 72;
  TWCR = _BV(TWEN); // enable I2C
}
