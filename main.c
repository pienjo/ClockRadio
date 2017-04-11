#include <avr/io.h>
#include <util/delay.h>
#include "Renderer.h"
#include "Panels.h"
#include "DateTime.h"
#include "DS1307.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>

struct DateTime TheDateTime;

#define BUTTON1_CLICK 0x0001 
#define BUTTON2_CLICK 0x0002 
#define CLOCK_UPDATE  0x0004 // At 4, because it should mask bit 2.
#define BUTTON3_CLICK 0x0008
#define BUTTON4_CLICK 0x0010
#define BUTTON5_CLICK 0x0020
#define BUTTON6_CLICK 0x0040
#define BUTTON7_CLICK 0x0080
#define BUTTON1_RELEASE 0x0100
#define BUTTON2_RELEASE 0x0200
#define CLOCK_TICK      0x0400
#define BUTTON3_RELEASE 0x0800
#define BUTTON4_RELEASE 0x1000
#define BUTTON5_RELEASE 0x2000
#define BUTTON6_RELEASE 0x4000
#define BUTTON7_RELEASE 0x8000


volatile uint16_t event;

ISR (INT0_vect) // External interrupt 0
{
  event |= CLOCK_UPDATE;
}

uint8_t timer2_scaler = 2;

ISR (TIMER2_OVF_vect)
{
  // Crude button debouncer
  static uint8_t buttonDebounce[4] = { 0 };

  // First 3 array members are the debounce queue
  // Last array member is the reported button state
  
  // Add new data
  buttonDebounce[timer2_scaler] = PIND;
  
  uint8_t buttonsToReport = ~((buttonDebounce[0] ^ buttonDebounce[1]) | (buttonDebounce[1] ^ buttonDebounce[2]));
  // Now holds all buttons whose state has been stable over the last 3 scans (all values equal)

  // Only report buttons that haven't been reported yet.
  buttonsToReport &= (buttonDebounce[timer2_scaler] ^ buttonDebounce[3]);
 
  buttonsToReport &= ~(0x02); // Exclude ext0

  if (buttonsToReport)
  {
    // Mark as handled
    buttonDebounce[3] =( buttonDebounce[3] & (~buttonsToReport) ) | (buttonsToReport & buttonDebounce[timer2_scaler]);
  
    // Propagate only press events (1->0 flanks, as buttons are pulled up when not pressed)
    uint8_t pressedButtons = buttonsToReport & ~(buttonDebounce[timer2_scaler]);
    uint8_t releasedButtons = buttonsToReport & (buttonDebounce[timer2_scaler]);
    
    // Report
    event |= (uint16_t) pressedButtons | ((uint16_t)releasedButtons << 8);
  }


  
  if (timer2_scaler == 0)
  {
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

  // Set port D to input, enable pull-up on portD except for PortD2 (ext0)

  DDRD = 0;
  PORTD = ~(_BV(PORTD2));

  // Setup external interrupt for the 1Hz clock
  
  EICRA = _BV(ISC01);
  EIMSK = _BV(INT0);

  TIMSK2 = _BV(TOIE2); // Enable overflow interrupt, will trigger every 16 ms - but don't start the timer yet.
  
  // Setup timer 2: /1024 prescaler, 
  TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); 

  uint8_t secMode = SECONDARY_MODE_SEC;

  while (1)
  {
    uint16_t eventToHandle = 0;

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      eventToHandle = event;
      event = 0;
    }
    
    if (eventToHandle & CLOCK_TICK)
    {
      Renderer_Tick();
    }

    if (eventToHandle & CLOCK_UPDATE)
    {
      Read_DS1307_DateTime();
      Renderer_Update(MAIN_MODE_TIME, secMode, true); 
    }
    
    if (eventToHandle & BUTTON1_CLICK)
    {
      secMode = SECONDARY_MODE_YEAR;
      Renderer_Update(MAIN_MODE_TIME, secMode, true); 
    }
    
    if (eventToHandle & BUTTON1_RELEASE)
    {
      secMode = SECONDARY_MODE_SEC;
      Renderer_Update(MAIN_MODE_TIME, secMode, true); 
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
