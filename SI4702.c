#include "i2c.h"
#include <avr/io.h>
#include "SI4702.h"
#include <stdbool.h>
#include <util/delay.h>

#define SI4702_ADDR 0x20

// The SI4702 has bigendian registers, the AVR is little endian. 
// Work around this by not addressing the registers as 16 bits - it's not really that useful
// to access them as 16 bits anyway - only the 9-bit tuning registers benefit from 16-bit
// access, but that's easily fixed

uint8_t SI4702_regs[32]; 

// Register definitions
#define POWERCONFIG_H 0x04
  #define DSMUTE  0x80 // Disable softmute
  #define DMUTE   0x40 // Disable mute
  #define MONO    0x20 // mono
  #define RDSM	  0x08 // RDS mode (not supported)
  #define SKMODE  0x04 // Seek mode: stop seeking at band end
  #define SEEKUP  0x02 // Seek up
  #define SEEK    0x01 // Start seeking

#define POWERCONFIG_L 0x05
  #define DISABLE 0x40 // Power up disable
  #define ENABLE  0x01 // Powerup enable

#define CHANNEL_H 0x06
  #define TUNE	   0x80 // Start tuning.
  #define CHANNEL_H_BITS 0x03 // Channel[8:9]

#define CHANNEL_L 0x07
  // Channel[7:0]

#define SYSCONFIG1_H 0x08
  #define RDSIEN  0x80 // RDS interrupt enable (not supported)
  #define STCIEN  0x40 // Seek/Tune interrupt enable (not supplied by breakout board)
  #define RDS     0x10 // RDS enable (not supported)
  #define DE      0x08 // De-emphasis. 0 for USA, 1 for rest of world
  #define AGCD    0x04 // Automatic Gain Control disable

#define SYSCONFIG1_L 0x09
  // Stereo/mono blend
  #define BLENDADJ_0 0x00 // +0 dB
  #define BLENDADJ_1 0x40 // +6 dB
  #define BLENDADJ_2 0x80 // -12 dB
  #define BLENDADJ_3 0xC0 // -6 dB
  #define BLENDADJ_BITS BLENDADJ_3

  // GPIO3 mode (not bonded out)
  #define GPIO3_HI_Z 0x00
  #define GPIO3_ST   0x10 // Stereo indicator
  #define GPIO3_LOW  0x20 // force low
  #define GPIO3_HIGH 0x30 // Force high
  #define GPIO3_BITS GPIO3_HIGH

  // GPIO2 mode (not bonded out)
  #define GPIO2_HI_Z 0x00
  #define GPIO2_INT  0x04 // Seek/Tune or RDS interrupt
  #define GPIO2_LOW  0x08 // force low
  #define GPIO2_HIGH 0x0C // Force high
  #define GPIO2_BITS GPIO2_HIGH

  // GPIO1 mode (not bonded out)
  #define GPIO1_HI_Z 0x00
  #define GPIO1_LOW  0x02 // force low
  #define GPIO1_HIGH 0x03 // Force high
  #define GPIO1_BITS GPIO1_HIGH

#define SYSCONFIG2_H  0x0a
  // bits 0:7 : seek threshold.
#define SYSCONFIG2_L  0x0b
  #define BAND_US_EU 0x00 // 87.5 - 108MHz (US / Europe / Australia)
  #define BAND_JP_WD 0x40 // 76 - 108 (Japan wide band)
  #define BAND_JP    0x80 // 76 - 89 MHz (japan)
  #define BAND_BITS  0xC0
 
  #define SPACE_200KHZ 0x00 // 200 KHz spacing (US /Australia)
  #define SPACE_100KHZ 0x10 // 100 KHz spacing (Europe/Japan)
  #define SPACE_50KHZ  0x20 // 50 KHz spacing
  #define SPACE_BITS   0x30 
  // bits 3:0 : Volume.
  #define VOLUME_BITS  0x0F

#define SYSCONFIG3_H  0x0c
  #define SMUTER_FASTEST   0x00 // Softmute attack/recovery rate
  #define SMUTER_FAST      0x40
  #define SMUTER_SLOW      0x80
  #define SMUTER_SLOWEST   0xC0
  #define SMUTER_BITS SMUTER_SLOWEST

  #define SMUTEA_0	   0x00 // 16dB
  #define SMUTEA_1         0x10 // 14 dB
  #define SMUTEA_2         0x20 // 12 dB
  #define SMUTEA_3         0x30 // 10 dB
  #define SMUTEA_BITS	   SMUTEA_3
  
  #define VOLEXT 0x01      // extended volume rage (output attenuates by 30 dB)

#define SYSCONFIG3_L   0x0d
  #define SKSNR(x)  (x << 4) // Seek SNR threshold (0 = most stops, 7 = fewest stops)
  #define SKSNR_BITS 0xf0
  #define SKCNT(x)  (x)
  #define SKCNT_BITS  0x0f

#define TEST1_H 0x0e
  #define XOSCEN 0x80       // Crystal oscillator enable
  #define AHIZEN 0x40       // Audio High-Z enable

#define TEST1_L 0x0f
  // reserved

#define TEST2_H 0x10    
  // reserved
#define TEST2_L 0x11    
  // reserved

#define BOOTCONFIG_H 0x12
  // reserved

#define BOOTCONFIG_L 0x13
  // reserved

#define STATUS_RSSI_H 0x14
  #define RDSR 0x80 // RDS ready
  #define STC  0x40 // Seek/tune complete
  #define SBFL 0x20 // Seek Fail / Band Limit
  #define AFCRL 0x10 // AFC fail
  #define RDSS 0x08 // RDS synchronized
  
  #define BLERA(x) ((x & 0x06) >> 1) // RDS block A errors
  #define ST 0x01   // Stereo

#define STATUS_RSSI_L 0x15
  // [0:7] : RSSI (REceived Signal Strength Indicator)

#define READ_CHANNEL_H 0x16
  #define BLERB(x) ((x & 0xC0) >> 6) // RDS Block B errors
  #define BLERC(x) ((x & 0x30) >> 4) // RDS Block C errors
  #define BLERD(x) ((x & 0xc0) >> 2) // RDS block D erros
  #define READ_CHANNEL_H_BITS 0x3 // ReadChannel[8:9]

#define READ_CHANNEL_L 0x17
  // ReadChannel[7:0]

void Write_SI4702();
void SI4702_SetFrequency_intern(uint16_t frequency) // Frequency in .1 MHz 
{
  if ((SI4702_regs[SYSCONFIG2_L] & BAND_BITS) != BAND_US_EU)
  {
    frequency -= 760;
  }
  else
  {
    frequency -= 875;
  }

  if((SI4702_regs[SYSCONFIG2_L] & SPACE_BITS) == SPACE_50KHZ)
    frequency *= 2;
  else if((SI4702_regs[SYSCONFIG2_L] & SPACE_BITS) == SPACE_200KHZ)
    frequency /= 2;

  SI4702_regs[CHANNEL_L] = frequency & 0xff;
  SI4702_regs[CHANNEL_H] = ( SI4702_regs[CHANNEL_H] & (~CHANNEL_H_BITS) ) | (frequency >> 8);


}

uint16_t SI4702_GetFrequency()
{
  if (! SI4702_regs[POWERCONFIG_L] & ENABLE )
    return 0;
  
  uint16_t frequency = SI4702_regs[READ_CHANNEL_H] & READ_CHANNEL_H_BITS;
  frequency <<= 8;
  frequency |= SI4702_regs[READ_CHANNEL_L];

  if((SI4702_regs[SYSCONFIG2_L] & SPACE_BITS) == SPACE_50KHZ)
    frequency /= 2;
  else if ((SI4702_regs[SYSCONFIG2_L] & SPACE_BITS) == SPACE_200KHZ)
    frequency *= 2;

  if ((SI4702_regs[SYSCONFIG2_L] & BAND_BITS) != BAND_US_EU)
  {
    frequency += 760;
  }
  else
  {
    frequency += 875;
  }

  return frequency;
}
// Set seek threshold (0-255). Higher values stop at a higher signal strength only
void SI4702_SetSeekThreshold(uint8_t threshold)
{
  SI4702_regs[SYSCONFIG2_H] = threshold;
}

// Set volume ( 0 = mute, 30 = max)
void SI4702_SetVolume(uint8_t volume)
{
  if (volume > 30)
    return;
    
  if (volume > 15)
  {
    // Disable VOLEXT
    volume -= 15;
    SI4702_regs[SYSCONFIG3_H] &= ~VOLEXT;
  }
  else
  {
    // enable VOLEXT
    SI4702_regs[SYSCONFIG3_H] |= VOLEXT;
  }
  SI4702_regs[SYSCONFIG2_L] &= ~VOLUME_BITS;
  SI4702_regs[SYSCONFIG2_L] |= (volume & 0x0f);
  
  Write_SI4702();
}

uint8_t SI4702_GetVolume()
{
  uint8_t ret = SI4702_regs[SYSCONFIG2_L] & VOLUME_BITS;
  if ((SI4702_regs[SYSCONFIG3_H] & VOLEXT) == 0)
    ret += 15;
  
  return ret;
}


inline void I2CWait()
{
  while (!(TWCR & _BV(TWINT)))
    ; // Wait
}

// The SI4702 has a very weird I2C interface. All registers
// are 16-bits wide (big endian), but for some bizarre reason
// start at address 0x0A, then auto-increment to 0x0F, and then
// wrap around to address 0

void Read_SI4702()
{
  uint8_t currentReg = 0x14; // 2 * 0x0a

  // Send START
  TWCR = _BV(TWINT)| _BV(TWSTA) | _BV(TWEN);

  I2CWait();

  if ((TWSR & 0xF8) != 0x08)
    goto error;
  
  // Send slave address, with read bit
  TWDR = SI4702_ADDR | 1;

  TWCR = _BV(TWEN) | _BV(TWINT);
  I2CWait();

  if ((TWSR & 0xf8) != 0x40)
    goto error; // Not acked

  do
  {
    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA); // Clear TWINT to resume, send ack to indicate more data is requested
    I2CWait();
    if((TWSR & 0xf8) != 0x50)
      goto error; // Not acked.

    SI4702_regs[currentReg++] = TWDR; // Store data.
   
    if (currentReg == 32) // wrap around
      currentReg = 0;

  } while (currentReg != 0x13 ); // Ack the first 31 (!) bytes
  
  TWCR = _BV(TWEN) | _BV(TWINT); // Clear TWINT to resume, send nack 
  I2CWait();

  SI4702_regs[currentReg++] = TWDR;

error:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); // Stop    
  // Wait on STOP to clear
  while ((TWCR & _BV(TWSTO)))
    ;
}

// Writing out the SI4072 registers is even more weird. According to the documentation,
// writeout starts at the high byte of register 0x2, auto-increments up to 0xf, and
// then wraps around to 0. However, address 8 and 9 may not be written...
// Basicly, this means only addresses 0x2 through 0x7 are available for writing

void Write_SI4702()
{
  // Can use Write_I2C_Regs here, by supplying the MSB of register 2 as "register address"
  Write_I2C_Regs(SI4702_ADDR, SI4702_regs[4], 11, SI4702_regs + 5); // Send MSB of 2 as "register adddress" and 11 remaining data bytes.
}

void Init_SI4702()
{
  // Disable I2C module

  _Bool i2cWasEnabled = false;
  if (TWCR & _BV(TWEN))
  {
    i2cWasEnabled = true;
    TWCR &= ~(_BV(TWEN));
  }

  PORTC &= ~(_BV(PORTC3)); // Clear port c3
  DDRC |= _BV(PORTC3); // Port C3 (reset input of SI4702) to output
  
  _delay_us(500); 
  // Clear port c4 (SDA)
  PORTC &= ~(_BV(PORTC4));
  
  _delay_us(500);  // Si4702 datasheet: > 100 us
  // assert C3 (SI4702 reset)
  DDRC &= ~_BV(PORTC3); // use external pull-up to 3v3
  
  _delay_us(10); // Si4702 datasheet: > 30 ns
  // Assert C4
  PORTC |= _BV(PORTC4); 

  _delay_us(10); // Si4702 datasheet: > 300 ns 

  if(i2cWasEnabled)
  {
    TWCR |= _BV(TWEN); // enable I2C
  }
  
  // Read current registers
  Read_SI4702();
  // Enable the oscillator
  SI4702_regs[TEST1_H] |= XOSCEN;
  // Writeout
  Write_SI4702();

  _delay_ms(125); // Allow oscillator to settle
}

void SI4702_PowerOn()
{
  Read_SI4702(); // Some registers may have shifted during takeoff
  SI4702_regs[POWERCONFIG_H] = DSMUTE | DMUTE | MONO;
  SI4702_regs[POWERCONFIG_L] = ENABLE ;

  SI4702_regs[SYSCONFIG1_H] |= DE;  // Use European  de-emphasis
  SI4702_regs[SYSCONFIG2_L]  = SPACE_100KHZ | BAND_US_EU;  // European spacing
  SI4702_SetSeekThreshold(20);
  Write_SI4702();
}

void SI4702_PowerOff()
{
  Read_SI4702(); // Some registers may have shifted during takeoff
  SI4702_regs[POWERCONFIG_L] |= DISABLE;
  Write_SI4702();
  _delay_ms(125); // Allow oscillator to settle
  Read_SI4702(); // Some registers may have shifted during takeoff
}

uint16_t targetFreq = 0;
enum {
    seekIdle,
    seekUp,
    seekDown,
    seekBusy,
  } seekMode = seekIdle;

void SI4702_SetFrequency(uint16_t freq)
{
  if (freq >= 875 && freq <= 1080)
    targetFreq = freq;
}

void SI4702_Seek(_Bool seekUp)
{
  if (seekMode == seekIdle)
  {
    if (seekUp)
      seekMode = seekUp;
    else
      seekMode = seekDown;
  }
}

void SI4702_Tune(_Bool seekUp)
{
  uint16_t freq = SI4702_GetFrequency();
  
  if (seekUp)
  {
    if (freq < 1080 )
      freq++;
    else
      freq=875;    
  }
  else
  {
    if (freq > 875 )
      freq--;
    else
      freq=1080;
  }
  
  targetFreq = freq ;
}

_Bool Poll_SI4702()
{
  _Bool returnValue = 0;
  
  Read_SI4702(); // Some registers may have shifted during takeoff
  
  if (SI4702_regs[STATUS_RSSI_H] & STC)
  {
    // Stop tuning
    SI4702_regs[CHANNEL_H] &= ~TUNE;
    SI4702_regs[POWERCONFIG_H] &=~(SEEK);
    seekMode = seekIdle;
    returnValue = 1;
  }
  else
  {
    if (targetFreq)
    {
      SI4702_SetFrequency_intern(targetFreq);
      targetFreq = 0;
      // Abort seek, should it be in progress      
      //SI4702_regs[POWERCONFIG_H] &= (SEEK);
      SI4702_regs[CHANNEL_H] |= TUNE;
    } 
    
    switch(seekMode)
    {
      case seekIdle:
      case seekBusy:
	break;
      case seekUp:
	SI4702_regs[POWERCONFIG_H] |= SEEKUP | SEEK;
	seekMode = seekBusy;
	break;
      case seekDown:
	SI4702_regs[POWERCONFIG_H] &= ~SEEKUP;
	SI4702_regs[POWERCONFIG_H] |= (SEEK);
	seekMode = seekBusy;
	break;
    }
  }
  
  Write_SI4702();
  
  return returnValue;
}
