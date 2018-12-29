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
#include "i2c.h"
#include <avr/io.h>
#include <util/delay.h>

#define I2C_RECOVERY_ATTEMPTS 5

static void I2CWait()
{
  while (!(TWCR & _BV(TWINT)))
    ; // Wait
}

#define MIN_NR_RECOVERY_BITS 9  // 8 data bits + ack.

static void _recovery()
{
  // Start request could not be sent. This means the bus is stuck, and needs to be recovered.
  
  TWCR = (1 << TWINT)|(1 << TWSTO); // According to the data sheet, this will release the bus.
  
  // Disable I2C module, and perform recovery procedure.
  TWCR &= ~_BV(TWEN);
  
  DDRC &= ~(_BV(PORTC4)); // Port C4 (SDA) to input
  DDRC |= _BV(PORTC5); // Port C5 (SDL) to output
  
  // Send pulses along SCL until at least MIN_NR_RECOVERY_BITS consecutive "ones" are read back.
  
  for( uint8_t recoveryPhase = MIN_NR_RECOVERY_BITS; recoveryPhase != 0; )
  {
    PORTC &= ~_BV(PORTC5); // SCL low
    _delay_us(5); // For 100 KHz clock
    PORTC |= _BV(PORTC5); // SCL high
    _delay_us(5); // For 100 KHz clock
    if (PORTC & _BV(PORTC4))
    {
      // Read back a one
      --recoveryPhase;
    }
    else
    {
      // Read back a zero, recovery not done
      recoveryPhase = MIN_NR_RECOVERY_BITS;
    }
  }
  
  Init_I2C();
}

static _Bool _doStart( const uint8_t addr) 
{  

  for (uint8_t attempt = 0; attempt < I2C_RECOVERY_ATTEMPTS; ++attempt)
  {
    // Send START
    TWCR = _BV(TWINT)| _BV(TWSTA) | _BV(TWEN);

    I2CWait();

    if ((TWSR & 0xF8) == 0x08) 
    {
      break;
    }
    
    _recovery();
  }
    
  TWDR= addr;  // Slave address

  TWCR = _BV(TWEN) | _BV(TWINT);
  I2CWait();

  if (addr & 1)
  {
    if ((TWSR & 0xf8) != 0x40)
      return 0;
  }
  else
  {
    if ((TWSR & 0xf8) != 0x18)
      return 0;
  }
  
  return 1;
}

static _Bool _Write_I2C_Bulk(uint8_t amount, const uint8_t *ptr)
{
  while(amount--)
  {
    TWDR = *ptr++;
    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA); // Clear TWINT to resume,
    I2CWait();
    if((TWSR & 0xf8) != 0x28)
      return 0;
  }
  
  return 1;
}

static _Bool _Read_I2C_Bulk(uint8_t amount, uint8_t *ptr)
{
  while(--amount)
  {
    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA); // Clear TWINT to resume, send ack to indicate more data is requested
    I2CWait();
    if((TWSR & 0xf8) != 0x50)
      return 0; // not acked

    *ptr++ = TWDR;
  }

  //read last byte

  TWCR = _BV(TWEN) | _BV(TWINT); // Clear TWINT to resume, send nack 
  I2CWait();
  *ptr++ = TWDR;

  return 1;
}

_Bool Write_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, const uint8_t *ptr)
{
  if (amount == 0)
    return 1;
    
  _Bool success = _doStart(addr & 0xfe);
  
  if (success)
  {
    // Send register address

    TWDR = reg;
    TWCR = _BV(TWEN) | _BV(TWINT);
    I2CWait();

    if ((TWSR & 0xf8) == 0x28)
    {      
      success = _Write_I2C_Bulk(amount, ptr);
    }
  }

  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); // Stop    
  // Wait on STOP to clear
  while ((TWCR & _BV(TWSTO)))
    ;
  return success;
}

_Bool Write_I2C_Raw(uint8_t addr, uint8_t amount, const uint8_t *ptr)
{
  if (amount == 0)
    return 1;
    
  _Bool success = _doStart(addr & 0xfe);
  
  if (success)
  {
    success = _Write_I2C_Bulk(amount, ptr);
  }
  
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); // Stop    
  // Wait on STOP to clear
  while ((TWCR & _BV(TWSTO)))
    ;
  return success;
}

_Bool Read_I2C_Regs(uint8_t addr, uint8_t reg, uint8_t amount, uint8_t *ptr)
{
  if (amount == 0)
    return 0;

  _Bool success = _doStart(addr & 0xfe); // slave, write in order to send register
  
  if (!success) 
  {
    goto error;
  }
  
  success = 0; // Until proven otherwise
  
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
  
  success = _Read_I2C_Bulk(amount, ptr);
  
error:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); // Stop    
  // Wait on STOP to clear
  while ((TWCR & _BV(TWSTO)))
    ;
  return success;
}

_Bool Read_I2C_Raw(uint8_t addr, uint8_t amount, uint8_t *ptr)
{
  if (amount == 0)
    return 0;

  _Bool success = _doStart(addr | 1 );
  
  if (success) // slave, read
  {
    success = _Read_I2C_Bulk(amount, ptr);
  }
  
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); // Stop    
  // Wait on STOP to clear
  while ((TWCR & _BV(TWSTO)))
    ;
  return success;
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
