#include "Panels.h"
#include <avr/io.h>
#include <stdlib.h>


enum MAX7219_registers
{
  MAX7219_DECODE_MODE = 0x09,
  MAX7219_INTENSITY   = 0x0a,
  MAX7219_SCANLIMIT   = 0x0b,
  MAX7219_SHUTDOWN    = 0x0c,
};

uint8_t *panelBitMask = 0;
static uint8_t nrPanels = 0;

static void MAX7219_WriteAll( uint8_t reg, uint8_t data)
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
  MAX7219_WriteAll( MAX7219_SHUTDOWN   , 1); // Enable panel
}

void SetBrightness(uint8_t level)
{
  MAX7219_WriteAll( MAX7219_INTENSITY  , level);
}

void UpdatePanel()
{
  const uint8_t *p = panelBitMask + nrPanels * 8 - 1;
  for (uint8_t y = 1; y < 9; ++y)
  {
    // Clear slave select
    PORTB &= ~_BV(PORTB2);

    for (uint8_t x = nrPanels; x >0 ; --x)
    {
      SPDR = y;
      while (! (SPSR & _BV(SPIF))) ; // Wait for completion
      SPDR = *p--;
      while (! (SPSR & _BV(SPIF))) ; // Wait for completion
    }

    // Set slave select
    PORTB |= _BV(PORTB2);
  }
}
