#include <avr/io.h>
#include <util/delay.h>
#include "TimeRenderer.h"
#include "Panels.h"
#include "DateTime.h"

#define TICKTIME 50

struct DateTime TheDateTime;

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
}

void Read_DS1307_DateTime()
{
  // Read the first 7 registers.

  Read_DS1307_regs(0, 7, (uint8_t *)&TheDateTime);
  
  // Check bit 7 of "sec" - if it is 1, the clock wasn't running!
  if (TheDateTime.sec & 0x80)
  {
    TheDateTime.sec &= 0x7f;
    _delay_ms(100); 
    Write_DS1307_regs(0,1,(uint8_t *)&TheDateTime); // Clear it
  }
}

int main(void)
{
  // Set up SPI outputs
  
  DDRB |= _BV(PORTB2) | _BV(PORTB3) | _BV(PORTB5);

  // Enable SPI, master mode, 1MHz clock
  SPCR |= _BV(SPE) | _BV(MSTR) | _BV(SPR0);

  // Set up I2C
  
  DDRC = 0; // entire port input
  PORTC = 0x3f; // Enable pull-up
  DDRC = 0x30; // PC4 and PC5 to output.

  // Configure I2C speed: 100 KHz @ 16MHz clock
  TWSR = 0;
  TWBR = 72;
  TWCR = _BV(TWEN); // enable I2C

  InitializePanels(3);
  SetBrightness(1);

  TimeRenderer_SetTime(99,99,false); // reset
  
  uint8_t trig = 0;
  
  while (1)
  {
    _delay_ms(TICKTIME); 
    TimeRenderer_Tick();
    trig = trig + 1;
    if (trig == (1000 / TICKTIME) )
    {
      Read_DS1307_DateTime();
      TimeRenderer_SetTime(TheDateTime.min,TheDateTime.sec,true); 
      trig = 0;
    }
  }
}
