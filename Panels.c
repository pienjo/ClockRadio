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

void InitializePanels(uint8_t numPanels)
{
  // Set up SPI outputs
  
  DDRB |= _BV(PORTB2) | _BV(PORTB3) | _BV(PORTB5);

  // Enable SPI, master mode, 1MHz clock
  SPCR |= _BV(SPE) | _BV(MSTR) | _BV(SPR0);

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

void SendRow(uint8_t row, const uint8_t *data)
{
  // Clear slave select
  PORTB &= ~_BV(PORTB2);

  // Panel data is sent in reverse: last byte first.
  for (int8_t x = nrPanels -1; x >= 0 ; --x)
  {
    SPDR = 8-row;
    while (! (SPSR & _BV(SPIF))) ; // Wait for completion
    SPDR = data[x];
    while (! (SPSR & _BV(SPIF))) ; // Wait for completion
  }
  // Set slave select
  PORTB |= _BV(PORTB2);
}

