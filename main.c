#include <avr/io.h>
#include <util/delay.h>
#include "Renderer.h"
#include "Panels.h"
#include "DateTime.h"
#include "DS1307.h"
#include "SI4702.h"

#include "i2c.h"

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>

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

#define SET_TICKS 20

#define EDIT_MODE_ONES     0x1
#define EDIT_MODE_TENS     0x2
#define EDIT_MODE_NUMBER   (EDIT_MODE_ONES |EDIT_MODE_TENS)
#define EDIT_MODE_MASK     0x03
#define EDIT_MODE_ONEBASE 0x04

enum clockMode
{
  modeShowTime,
  modeAdjustYearTens,
  modeAdjustYearOnes,
  modeAdjustMonth,
  modeAdjustDayTens,
  modeAdjustDayOnes,
  modeAdjustHoursTens,
  modeAdjustHoursOnes,
  modeAdjustMinsTens,
  modeAdjustMinsOnes,
  modeAdjustDone,
  modeCount,
};

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

const uint8_t PROGMEM dpm[] =
{
  0x31, // jan
  0x28, // feb
  0x31, // mar
  0x30, // apr
  0x31, // may
  0x30, // jun
  0x31, // jul
  0x31, // aug
  0x30, // sep
  0x31, // oct
  0x30, // nov
  0x31, // dec;
};

uint8_t GetDaysPerMonth()
{
  uint8_t days = pgm_read_byte(dpm + TheDateTime.month-1);
  if (TheDateTime.month == 2 && (TheDateTime.year %4 == 0))
    days++; // February on a leap year
  
  return days;
}

const uint8_t PROGMEM dowTable[] = { 0,3,2,5,0,3,5,1,4,6,2,4 };
void UpdateDOW()
{
  uint8_t year = TheDateTime.year >> 4 | (TheDateTime.year & 0xf); // Undo BCD
  TheDateTime.wday = ((year + 5 - year / 4 - pgm_read_byte(dowTable + TheDateTime.month -1) + TheDateTime.day) % 7) + 1;
}

void AmplifierOn()
{
	// Amplifier control is active low
	PORTC = PORTC & ~( _BV(PORTC2));
}

void AmplifierOff()
{
	// Amplifier control is active low
	PORTC = PORTC | _BV(PORTC2);
}

int main(void)
{
	// Enable output on PORT C2 (amplifier control)
  DDRC |= _BV(PORTC2);
  AmplifierOff();
  
  InitializePanels(4);
  SetBrightness(1);
  
  Init_I2C();
  Init_DS1307();
  Init_SI4702();
  SI4702_SetFrequency(886);

  // Set port D to input, enable pull-up on portD except for PortD2 (ext0)

  DDRD = 0;
  PORTD = ~(_BV(PORTD2));

  // Setup external interrupt for the 1Hz clock
  
  EICRA = _BV(ISC01);
  EIMSK = _BV(INT0);

  TIMSK2 = _BV(TOIE2); // Enable overflow interrupt, will trigger every 16 ms - but don't start the timer yet.
  
  // Setup timer 2: /1024 prescaler, 
  TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); 

  uint8_t secMode = SECONDARY_MODE_RADIO;
  uint8_t mainMode = MAIN_MODE_TIME;

  uint8_t setTimeout = 0;
  enum clockMode deviceMode = modeShowTime;

  uint8_t *editDigit = 0;
  uint8_t editMode = 0;
  uint8_t editMaxValue = 0x99;
  
  uint8_t amplifier_tick = 0;
  
  while (1)
  {
    uint16_t eventToHandle = 0;

    _Bool updateScreen = 0;

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      eventToHandle = event;
      event = 0;
    }
    enum clockMode newDeviceMode = deviceMode;

    if (eventToHandle & CLOCK_TICK)
    {
      Renderer_Tick();
      if(setTimeout && setTimeout < SET_TICKS)
      {
        setTimeout++;
        if(setTimeout == SET_TICKS)
        {
          newDeviceMode++;
        }
      }
    }
    
    if (eventToHandle & BUTTON1_CLICK)
    {
      if (deviceMode == modeShowTime)
        setTimeout = 1;
      else
      {
        newDeviceMode++;
      }
    }

    if (newDeviceMode != deviceMode)
    {
      // Update the screen.
      switch(newDeviceMode)
      {
        case modeAdjustYearTens:
          mainMode = MAIN_MODE_DATE;
          secMode = SECONDARY_MODE_YEAR;
          updateScreen = 1;

          Renderer_SetFlashMask(0x2); 
          editDigit = &TheDateTime.year;
          editMode = EDIT_MODE_TENS | EDIT_MODE_ONEBASE;
          editMaxValue = 0x99 | EDIT_MODE_ONEBASE;
          break;
        case modeAdjustYearOnes:
          Renderer_SetFlashMask(0x1); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustMonth:
          editMode = EDIT_MODE_NUMBER | EDIT_MODE_ONEBASE; 
          editDigit = &TheDateTime.month;
          editMaxValue = 0x12;
          Renderer_SetFlashMask(0x30); 
          break;
        case modeAdjustDayTens:
          editDigit = &TheDateTime.day;
          editMaxValue = GetDaysPerMonth();
          editMode = EDIT_MODE_TENS | EDIT_MODE_ONEBASE;
          Renderer_SetFlashMask(0x80); 
          break;
        case modeAdjustDayOnes:
          editMode = EDIT_MODE_ONES | EDIT_MODE_ONEBASE;
          Renderer_SetFlashMask(0x40); 
          break;
        case modeAdjustHoursTens:
          mainMode = MAIN_MODE_TIME;
          secMode = SECONDARY_MODE_SEC;
          editMode = EDIT_MODE_TENS;
          updateScreen = 1;
          editMaxValue = 0x23;
          editDigit = &TheDateTime.hour;
          Renderer_SetFlashMask(0x80); 
          break;
        case modeAdjustHoursOnes:
          Renderer_SetFlashMask(0x40); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustMinsTens:
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x59;
          editDigit = &TheDateTime.min;
          Renderer_SetFlashMask(0x20); 
          break;
        case modeAdjustMinsOnes:
          Renderer_SetFlashMask(0x10); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustDone:
          Renderer_SetFlashMask(0);
          editMode = 0;
          // Write out time
          Write_DS1307_DateTime();
          newDeviceMode = modeShowTime;
          break;
        default:
          break;
      }
      deviceMode = newDeviceMode;
    }
    else
    {
	
      if (eventToHandle & CLOCK_UPDATE && deviceMode == modeShowTime)
      {
  			amplifier_tick ++;
				if (amplifier_tick & 0x4)
				{
					AmplifierOn();
				}
				else
				{
					AmplifierOff();
				}
        Read_DS1307_DateTime();
        Poll_SI4702();
        updateScreen = 1;
      }
    } 
    
    if (eventToHandle & BUTTON1_RELEASE)
    {
      setTimeout = 0;
    }
    
    if (editMode)
    {
      if ((eventToHandle & BUTTON2_CLICK || eventToHandle & BUTTON4_CLICK))
      {
        // up

        switch(editMode & EDIT_MODE_MASK)
        {
          case EDIT_MODE_ONES:
          {
            uint8_t v = *editDigit;
            v = v&0x0f;
            if (v < 9)
              v++;
            *editDigit = (*editDigit & 0xf0) | v;
            break;
          }
          case EDIT_MODE_TENS:
          {
            uint8_t v = *editDigit;
            v = v&0xf0;
            if (v < 0x90)
              v+= 0x10;
            *editDigit = (*editDigit & 0x0f) | v;
            break;
          }
          case EDIT_MODE_NUMBER:
          {
            (*editDigit)++;
            if ((*editDigit & 0x0f) == 0x0a)
            {
              *editDigit += 6; 
            }
            break;
          }
        }
        
        if (*editDigit > editMaxValue)
          *editDigit = editMaxValue;

        updateScreen = 1;
        UpdateDOW();
      }

      if (eventToHandle & BUTTON3_CLICK)
      {
        // down
        switch(editMode & EDIT_MODE_MASK)
        {
          case EDIT_MODE_ONES:
          {
            uint8_t v = *editDigit;
            v = v&0x0f;
            if (v > 0)
              v--;
            *editDigit = (*editDigit & 0xf0) | v;
            break;
          }
          case EDIT_MODE_TENS:
          {
            uint8_t v = *editDigit;
            v = v&0xf0;
            if (v > 0x00)
              v-= 0x10;
            *editDigit = (*editDigit & 0x0f) | v;
            break;
          }
          case EDIT_MODE_NUMBER:
          {
            if (*editDigit > 0)
            {
              (*editDigit)--;
              if ((*editDigit & 0x0f) == 0x0f)
              {
                *editDigit -= 6; 
              }
            }
            break;
          }
        }
        
        if ((editMode & EDIT_MODE_ONEBASE) && (*editDigit == 0))
          *editDigit = 1;
        
        UpdateDOW();
        updateScreen = 1;
      }
    }

    if (updateScreen)
    {
      // Something happened, update display.
      Renderer_Update(mainMode, secMode,(eventToHandle & CLOCK_UPDATE) && (deviceMode == modeShowTime) );
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
