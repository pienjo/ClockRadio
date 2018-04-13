#include "i2c.h"
#include <avr/io.h>
#include "SI4702.h"
#include <stdbool.h>
#include <util/delay.h>

#define SI4702_ADDR 0x20
#define SI4702_RECOVERY_ATTEMPTS 3

// The SI4702 has bigendian registers, the AVR is little endian. 
// Work around this by not addressing the registers as 16 bits - it's not really that useful
// to access them as 16 bits anyway - only the 9-bit tuning registers benefit from 16-bit
// access, but that's easily fixed

// The SI4702 has a very weird I2C interface. For some bizarre reason, Read-out starts at address 
// 0x0A, then auto-increment to 0x0F, and then wraps around to address 0. Write-out starts at 
// register adddress 2, and since address 8 and 9 may not be written to is restricted to registers
// 2 through 7. 

// Work around the read-out wraparound by re-arranging the registers so that register 0X0a is relocated
// to 0x00/0x01, 0x0f is relocated to 0x0a/0x0b, the wrapped-around address '0' moves to 0x0c/0x0d, 
// register 2 (write-out start address) sits at 0x10/0x11 and register 9 sits at 0x1E/0X1F

// Registers cannot be accessed directly, so there's no reason to keep the weird original layout. 
#define RELOCATED_REGISTER_2 POWERCONFIG_H

uint8_t SI4702_regs[32]; 

// Register definitions

// REGISTER A
#define STATUS_RSSI_H 0x00
  #define RDSR 0x80 // RDS ready
  #define STC  0x40 // Seek/tune complete
  #define SBFL 0x20 // Seek Fail / Band Limit
  #define AFCRL 0x10 // AFC fail
  #define RDSS 0x08 // RDS synchronized
  
  #define BLERA(x) ((x & 0x06) >> 1) // RDS block A errors
  #define ST 0x01   // Stereo

#define STATUS_RSSI_L 0x01
  // [0:7] : RSSI (REceived Signal Strength Indicator)

// REGISTER B
#define READ_CHANNEL_H 0x02
  #define BLERB(x) ((x & 0xC0) >> 6) // RDS Block B errors
  #define BLERC(x) ((x & 0x30) >> 4) // RDS Block C errors
  #define BLERD(x) ((x & 0xc0) >> 2) // RDS block D erros
  #define READ_CHANNEL_H_BITS 0x3 // ReadChannel[8:9]

#define READ_CHANNEL_L 0x03
  // ReadChannel[7:0]

// REGISTER C
#define RDSA_H   0x04
#define RDSA_L   0x05

// REGISTER D
#define RDSB_H   0x06
#define RDSB_L   0x07

// REGISTER E
#define RDSC_H   0x08
#define RDSC_L   0x09

// REGISTER F
#define RDSD_H   0x0a
#define RDSD_L   0x0b

// REGISTER 0
#define DEVICEID_H     0x0c
  #define PN_BITS      0xF0 // Part number mask
  #define MFGID_H_BITS 0x0F // Bits 8-11 of manufacturer ID.

#define DEVICEID_L     0x0d
  #define MFGID_L_BITS 0xff // bits 0-7 of manufacturer ID
  
// REGISTER 1
#define CHIPID_H       0x0e
  #define REV_BITS     0xFC // Revision (?) bits
  #define DEV_H_BITS   0x03
#define CHIPD_L	       0x0f
  #define DEV_L_BITS   0xC0
  #define FIRMWARE     0x3F
  
// REGISTER 2
#define POWERCONFIG_H  0x10

  #define DSMUTE  0x80 // Disable softmute
  #define DMUTE   0x40 // Disable mute
  #define MONO    0x20 // mono
  #define RDSM	  0x08 // RDS mode (not supported)
  #define SKMODE  0x04 // Seek mode: stop seeking at band end
  #define SEEKUP  0x02 // Seek up
  #define SEEK    0x01 // Start seeking

#define POWERCONFIG_L 0x11
  #define DISABLE 0x40 // Power up disable
  #define ENABLE  0x01 // Powerup enable

// REGISTER 3
#define CHANNEL_H 0x12
  #define TUNE	   0x80 // Start tuning.
  #define CHANNEL_H_BITS 0x03 // Channel[8:9]

#define CHANNEL_L 0x13
  // Channel[7:0]

// REGISTER 4
#define SYSCONFIG1_H 0x14
  #define RDSIEN  0x80 // RDS interrupt enable (not supported)
  #define STCIEN  0x40 // Seek/Tune interrupt enable (not supplied by breakout board)
  #define RDS     0x10 // RDS enable (not supported)
  #define DE      0x08 // De-emphasis. 0 for USA, 1 for rest of world
  #define AGCD    0x04 // Automatic Gain Control disable

#define SYSCONFIG1_L 0x15
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

// REGISTER 5
#define SYSCONFIG2_H  0x16
  // bits 0:7 : seek threshold.
#define SYSCONFIG2_L  0x17
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

// REGISTER 6
#define SYSCONFIG3_H  0x18
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

#define SYSCONFIG3_L   0x19
  #define SKSNR(x)  (x << 4) // Seek SNR threshold (0 = most stops, 7 = fewest stops)
  #define SKSNR_BITS 0xf0
  #define SKCNT(x)  (x)
  #define SKCNT_BITS  0x0f


// REGISTER 7
#define TEST1_H 0x1a
  #define XOSCEN 0x80       // Crystal oscillator enable
  #define AHIZEN 0x40       // Audio High-Z enable

#define TEST1_L 0x1b
  // reserved

// REGISTER 8
#define TEST2_H 0x1c    
  // reserved
#define TEST2_L 0x1d    
  // reserved

// REGISTER 9
#define BOOTCONFIG_H 0x1e
  // reserved

#define BOOTCONFIG_L 0x1f
  // reserved

static inline _Bool Read_SI4702()
{
  return Read_I2C_Raw(SI4702_ADDR, 32, SI4702_regs);
}

static inline _Bool Write_SI4702()
{
  return Write_I2C_Raw(SI4702_ADDR, 12, SI4702_regs + RELOCATED_REGISTER_2); 
}

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
  if ((SI4702_regs[POWERCONFIG_L] & ENABLE) == 0 )
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

_Bool Init_SI4702()
{
  for(uint8_t attempt = 0; attempt < SI4702_RECOVERY_ATTEMPTS; ++attempt)
  {
    // Disable I2C module
    TWCR &= ~(_BV(TWEN));

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

    TWCR |= _BV(TWEN); // enable I2C
    
    // Read current registers
    if (!Read_SI4702())
      continue;
      
    // Enable the oscillator
    SI4702_regs[TEST1_H] |= XOSCEN;
    // Writeout
    if (!Write_SI4702())
      continue;

    _delay_ms(125); // Allow oscillator to settle
    if (Read_SI4702())
      return 1; // Success!
  }
  
  return 0; // Error 
}

_Bool SI4702_PowerOn()
{
  if (!Read_SI4702()) // Some registers may have shifted during takeoff
  {
    if (!Init_SI4702()) // try to restart.
      return 0; // Restart failed
  }
  
  SI4702_regs[POWERCONFIG_H] = DSMUTE | DMUTE | MONO;
  SI4702_regs[POWERCONFIG_L] = ENABLE ;

  SI4702_regs[SYSCONFIG1_H] |= DE;  // Use European  de-emphasis
  SI4702_regs[SYSCONFIG2_L]  = SPACE_100KHZ | BAND_US_EU;  // European spacing
  SI4702_SetSeekThreshold(20);
  
  return Write_SI4702();
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
