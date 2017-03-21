#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

uint8_t *panelBitMask = 0;
uint8_t nrPanels = 0;

enum MAX7219_registers
{
  MAX7219_DECODE_MODE = 0x09,
  MAX7219_INTENSITY   = 0x0a,
  MAX7219_SCANLIMIT   = 0x0b,
  MAX7219_SHUTDOWN    = 0x0c,
};

void MAX7219_WriteAll( uint8_t reg, uint8_t data)
{
  
  // Clear slave select
  PORTB &= ~_BV(PORTB2);
 
  // Alternate register and data bits, for every panel
  for (uint8_t i = 0; i < nrPanels; ++i)
  {
    SPDR = reg;
    while (! (SPSR & _BV(SPIF))) ; // Wait for completion
    SPDR = data;
    while (! (SPSR & _BV(SPIF))) ; // Wait for completion
  }
  // Set slave select
  PORTB |= _BV(PORTB2);
}

void InitializePanel(uint8_t numPanels)
{
  panelBitMask = calloc(numPanels , 8); // 8 bytes per panel

  nrPanels = numPanels;

  // Configure panels.

  MAX7219_WriteAll( MAX7219_DECODE_MODE, 0); // No decode.
  MAX7219_WriteAll( MAX7219_SCANLIMIT  , 7); // Scan all rows
  MAX7219_WriteAll( MAX7219_INTENSITY  , 0); // Half brightness. 
  MAX7219_WriteAll( MAX7219_SHUTDOWN   , 1); // Enable panel
}

void UpdatePanel()
{
  const uint8_t *p = panelBitMask;
  for (uint8_t y = 1; y <= 8; ++y)
  {
    // Clear slave select
    PORTB &= ~_BV(PORTB2);

    for (uint8_t x = 0; x < nrPanels ; ++x)
    {
      SPDR = y;
      while (! (SPSR & _BV(SPIF))) ; // Wait for completion
      SPDR = *p++;
      while (! (SPSR & _BV(SPIF))) ; // Wait for completion
    }

    // Set slave select
    PORTB |= _BV(PORTB2);
  }
}
int main(void)
{
  // Set up SPI outputs
  
  DDRB |= _BV(PORTB2) | _BV(PORTB3) | _BV(PORTB5);

  // Enable SPI, master mode.
  SPCR |= _BV(SPE) | _BV(MSTR) | _BV(SPR1);
 
  InitializePanel(1);

  // Some test patterns
  panelBitMask[0] = 0xff; // All on
  panelBitMask[1] = 0xaa;
  panelBitMask[2] = 0x81;
  panelBitMask[3] = 0x55;
  panelBitMask[4] = 0x0f;
  panelBitMask[5] = 0x3c;
  panelBitMask[6] = 0xf0;
  panelBitMask[7] = 0x33;
  
  while (1)
  {
    _delay_ms(500); 
    UpdatePanel();
  }
}
