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
static uint8_t brightness = 4;

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
  MAX7219_WriteAll( MAX7219_INTENSITY, brightness);
}

void SetBrightness(uint8_t level)
{
  brightness = level;
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

