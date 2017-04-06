#include <avr/io.h>
#include <util/delay.h>
#include "TimeRenderer.h"
#include "Panels.h"
#include "DateTime.h"
#include "DS1307.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>

struct DateTime TheDateTime;

#define CLOCK_UPDATE 1
#define CLOCK_TICK   2
volatile uint8_t event;

ISR (INT0_vect) // External interrupt 0
{
  event |= CLOCK_UPDATE;
}

uint8_t timer2_scaler = 2;

ISR (TIMER2_OVF_vect)
{
  if (timer2_scaler == 0)
  {
    // Stop the tick timer.
    TCCR2B = 0;
    event |= CLOCK_TICK; // results in ~48 ms per call
    timer2_scaler = 2;
  }
  else
  {
    timer2_scaler--;
  }
}

int main(void)
{
  InitializePanels(4);
  SetBrightness(1);

  Init_DS1307();

  // Setup external interrupt for the 1Hz clock
  
  EICRA = _BV(ISC01);
  EIMSK = _BV(INT0);

  TIMSK2 = _BV(TOIE2); // Enable overflow interrupt, will trigger every 16 ms - but don't start the timer yet.

  TimeRenderer_SetTime(TheDateTime.min,TheDateTime.sec,TheDateTime.sec, false); 
  
  while (1)
  {
    uint8_t eventToHandle = 0;
    _Bool tickWanted = false;

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      eventToHandle = event;
      event = 0;
    }
    
    if (eventToHandle & CLOCK_TICK)
    {
      tickWanted |= TimeRenderer_Tick();
    }
    if (eventToHandle & CLOCK_UPDATE)
    {
      Read_DS1307_DateTime();
      tickWanted |= TimeRenderer_SetTime(TheDateTime.hour,TheDateTime.min, TheDateTime.sec,true); 
    }

    if (tickWanted)
    {
      // Setup timer 2: /1024 prescaler, 
      TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); 
    }
    
    if (eventToHandle == 0)
    {
      // Nothing to do, go to sleep
      set_sleep_mode(SLEEP_MODE_PWR_SAVE);
      cli();
      sleep_enable();
      sleep_bod_disable();
      sei();
      sleep_cpu();
      sleep_disable();
    }
  }
}
