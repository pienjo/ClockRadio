#include <avr/io.h>
#include <util/delay.h>
#include "TimeRenderer.h"
#include "Panels.h"

#define TICKTIME 50

int main(void)
{
  // Set up SPI outputs
  
  DDRB |= _BV(PORTB2) | _BV(PORTB3) | _BV(PORTB5);

  // Enable SPI, master mode, 1MHz clock
  SPCR |= _BV(SPE) | _BV(MSTR) | _BV(SPR0);
 
  InitializePanels(3);
  SetBrightness(1);

  uint8_t hours = 12, mins = 34;

  TimeRenderer_SetTime(hours,mins,false); // reset
  
  uint8_t trig = 0;
  
  while (1)
  {
    _delay_ms(TICKTIME); 
    TimeRenderer_Tick();
    trig = trig + 1;
    if (trig == (1000 / TICKTIME) )
    {
      trig = 0;
      mins = mins + 1;
      if (mins == 60)
      {
	mins = 0;
	hours++;
      }
      TimeRenderer_SetTime(hours,mins,true); 
    }
  }
}
