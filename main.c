#include <avr/io.h>
#include <util/delay.h>
#include "Renderer.h"
#include "Panels.h"
#include "DateTime.h"
#include "events.h"
#include "longpress.h"
#include "DS1307.h"
#include "SI4702.h"

#include "i2c.h"

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>

struct DateTime TheDateTime;

#define EDIT_MODE_ONES     0x1
#define EDIT_MODE_TENS     0x2
#define EDIT_MODE_NUMBER   (EDIT_MODE_ONES |EDIT_MODE_TENS)
#define EDIT_MODE_MASK     0x03
#define EDIT_MODE_ONEBASE 0x04

#define DS1307_RADIOSETTINGS 0
#define DS1307_BRIGHTNESS    3

#define BEEP_MARK	     3
#define BEEP_SPACE           4
#define BEEP_COUNT           4
#define BEEP_ON_PERIOD       (BEEP_COUNT * (BEEP_MARK + BEEP_SPACE))
#define BEEP_PAUSE	     BEEP_ON_PERIOD
#define BEEP_TOTALPERIOD     (BEEP_PAUSE + BEEP_ON_PERIOD)

uint8_t beepState = 0;

_Bool radioIsOn = 0;
_Bool beepIsOn = 0;

struct RadioSettings
{
  uint16_t frequency;
  uint8_t  volume;
};

enum clockMode
{
  modeShowTime,
  modeShowDate,
  modeShowRadio,
  modeShowRadio_Volume,
  modeAdjustYearTens,
  modeAdjustYearOnes,
  modeAdjustMonth,
  modeAdjustDayTens,
  modeAdjustDayOnes,
  modeAdjustHoursTens,
  modeAdjustHoursOnes,
  modeAdjustMinsTens,
  modeAdjustMinsOnes,
} ;

volatile uint16_t event ;

uint8_t timer2_scaler = 2;

ISR (TIMER2_OVF_vect)
{
  // Crude button debouncer
  static uint8_t buttonDebounce[4] = { 0 };

  // First 3 array members are the debounce queue
  // Last array member is the reported button state
  
  // Add new data
  buttonDebounce[timer2_scaler] = PIND;
  
  // Handle beeping here, so timing is strict
  if (beepIsOn)
  {
    beepState = beepState + 1;
    
    if (beepState >= BEEP_TOTALPERIOD)
      beepState = 0;
    
    if (beepState < BEEP_ON_PERIOD)
    {
      uint8_t remainder = beepState;
      while(remainder >= (BEEP_MARK + BEEP_SPACE))
	remainder -= BEEP_MARK + BEEP_SPACE;
	
      if (remainder == 0)
      {
	// Turn on beeper
	TCCR0B = _BV(CS01) | _BV(CS00);
      }
      else if (remainder == BEEP_MARK)
      {
	// Turn off beeper
	TCCR0B = 0;
	PORTC &= ~(_BV(PORTC1));
      }
    }
  }
  
  uint8_t buttonsToReport = ~((buttonDebounce[0] ^ buttonDebounce[1]) | (buttonDebounce[1] ^ buttonDebounce[2]));
  // Now holds all buttons whose state has been stable over the last 3 scans (all values equal)

  // Only report buttons that haven't been reported yet.
  buttonsToReport &= (buttonDebounce[timer2_scaler] ^ buttonDebounce[3]);
 
  if (buttonsToReport)
  {
    // Mark as handled
    buttonDebounce[3] =( buttonDebounce[3] & (~buttonsToReport) ) | (buttonsToReport & buttonDebounce[timer2_scaler]);
  
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

ISR (TIMER0_COMPA_vect)
{
  PINC = _BV(PORTC1); // Toggle output
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
 
void BeepOn()
{
  beepIsOn = 1;
  beepState = BEEP_ON_PERIOD;
  
  if (radioIsOn)
  {
    // Mute radio
  }
  else
  {
    // Amplifier control is active low
    PORTC = PORTC & ~( _BV(PORTC2)); 
  }
}

void BeepOff()
{
  TCCR0B = 0; // Stop timer
  beepIsOn = 0;
  
  // Force beeper output low
  PORTC &= ~ _BV(PORTC1);
  
  if (radioIsOn) // Turn off amplifier if radio is off
  {
    // Unmute radio.
  }
  else
  {
    // Amplifier control is active low
    PORTC = PORTC | _BV(PORTC2); 
  }
}

void RadioOn()
{
  // Retrieve previous channel and volume
  struct RadioSettings settings;
  
  Read_DS1307_RAM((uint8_t *)&settings, DS1307_RADIOSETTINGS, sizeof(settings));
  
  SI4702_PowerOn();
  
  SI4702_SetFrequency(settings.frequency);
  SI4702_SetVolume(settings.volume);
  
  Poll_SI4702();
  // Amplifier control is active low
  if (beepIsOn)
  {
    // Mute ratio
  }
  
  PORTC = PORTC & ~( _BV(PORTC2)); 
  radioIsOn = 1;  
}

void RadioOff()
{
  if (!beepIsOn)
  {
    // Amplifier control is active low
    PORTC = PORTC | _BV(PORTC2);
    // Store current frequency and volume
  }  
  
  struct RadioSettings settings;
  settings.frequency = SI4702_GetFrequency();
  settings.volume = SI4702_GetVolume();
  
  Write_DS1307_RAM((uint8_t *)&settings, DS1307_RADIOSETTINGS, sizeof(settings));
  
  SI4702_PowerOff();
  radioIsOn = 0;
}

void HandleEditUp(const uint8_t editMode, uint8_t *const editDigit, const uint8_t editMaxValue)
{
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
}

void HandleEditDown(const uint8_t editMode, uint8_t *const editDigit, const uint8_t editMaxValue)
{
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
} 

int main(void)
{
  // Enable output on PORT C2 (amplifier control) and C1 (beeper)
  DDRC |= _BV(PORTC2) | _BV(PORTC1);
  // Amplifier control is active low
  PORTC = PORTC | _BV(PORTC2);
  
  InitializePanels(4);
  
  Init_I2C();
  Init_DS1307();
  Init_SI4702();
  
  uint8_t brightness;
  Read_DS1307_RAM(&brightness, DS1307_BRIGHTNESS, 1);
  brightness = brightness & 0x0f; 
  SetBrightness(brightness);
  
  // Set port D to input, enable pull-up on portD except for PortD2 (ext0)

  DDRD = 0;
  PORTD = ~(_BV(PORTD2));

  TIMSK2 = _BV(TOIE2); // Enable overflow interrupt, will trigger every 16 ms
  
  // Setup timer 2: /1024 prescaler, 
  TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); 

  // Setup beeper timer.
  TCCR0A = _BV(WGM01); // CTC mode.  
  // Target frequency: 1600 Hz, two interrupts per cycle, or 5000 clock ticks.
  OCR0A = 78; // To be used with a /64 clock scaler
  TIMSK0 = _BV(OCIE0A); // Enable output match interrupt
  
  uint8_t secMode = SECONDARY_MODE_SEC;
  uint8_t mainMode = MAIN_MODE_TIME;

  enum clockMode deviceMode = modeShowTime;

  uint8_t *editDigit = 0;
  uint8_t editMode = 0;
  uint8_t editMaxValue = 0x99;
  uint8_t modeTimeout = 0;
  
  _Bool timePollAllowed = 1;
  
  sei(); // Enable interrupts. This will immediately trigger a port change interrupt; sink these events.
  
  while (1)
  {
    uint16_t eventToHandle = 0;

    _Bool updateScreen = 0;

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      eventToHandle = event;
      event = 0;
    }
    
    struct longPressResult longPressEvent = GetLongPress(eventToHandle);
    
    enum clockMode newDeviceMode = deviceMode;

    if (eventToHandle & CLOCK_TICK)
    {
      if (radioIsOn && Poll_SI4702() && deviceMode == modeShowRadio)
      {
	Renderer_Update_Secondary();
      }
      
      Renderer_Tick(secMode);
    }
    
    if (eventToHandle & CLOCK_UPDATE && timePollAllowed)
    {
      Read_DS1307_DateTime();
      updateScreen = 1;
    }
    
    // Handle keypresses
    switch ( deviceMode )
    {
      case modeShowTime:
      {
	if (eventToHandle & BUTTON2_CLICK)
	{
	  MarkLongPressHandled(BUTTON2_CLICK);
	  if (radioIsOn)
	  {
	    RadioOff();
	  }
	  else
	  {
	    RadioOn();
	    newDeviceMode = modeShowRadio;
	  }
	}
	if (longPressEvent.longPress & BUTTON1_CLICK)
	{
	  // adjust time
	  newDeviceMode = modeAdjustYearTens;
	  timePollAllowed = 0;
	} else if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  modeTimeout = 3;
	  newDeviceMode = modeShowDate;
	} else if (brightness != 0 && longPressEvent.repPress & BUTTON3_CLICK)
	{
	  brightness--;
	  Write_DS1307_RAM(&brightness, DS1307_BRIGHTNESS, 1);
	  SetBrightness(brightness);
	} else if (brightness < 15 && longPressEvent.repPress & BUTTON4_CLICK)
	{
	  brightness++;
	  Write_DS1307_RAM(&brightness, DS1307_BRIGHTNESS, 1);
	  SetBrightness(brightness);
	}
	break;
      }
      case modeShowDate:
      {
	if ((eventToHandle & CLOCK_UPDATE) && (--modeTimeout == 0) )
	{
	  newDeviceMode = modeShowTime;
	}
	
	if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  newDeviceMode = (radioIsOn? modeShowRadio : modeShowTime);
	}
	break;
      }
      case modeShowRadio:
      case modeShowRadio_Volume:
      {
	
	if (longPressEvent.shortPress & BUTTON2_CLICK)
	{
	  RadioOff();
	  newDeviceMode = modeShowTime;
	  break;
	}
	
	if (eventToHandle & BUTTON1_CLICK)
	{
	  MarkLongPressHandled(BUTTON1_CLICK);
	  newDeviceMode = modeShowTime;
	  break;
	} 
	
	if (PIND & BUTTON2_CLICK) 
	{
	  // Radio button is not pressed, use up-down keys for tuning
	  newDeviceMode = modeShowRadio;
	  if (radioIsOn)
	  {
	    if (eventToHandle & BUTTON3_CLICK)
	    {
	      SI4702_Tune(0);
	    } else if (eventToHandle & BUTTON4_CLICK)
	    {
	      SI4702_Tune(1);
	    } else if (longPressEvent.longPress & BUTTON3_CLICK)
	    {
	      SI4702_Seek(0);
	    } else if (longPressEvent.longPress & BUTTON4_CLICK)
	    {
	      SI4702_Seek(1);
	    }
	  }
	} else
	{
	  // Radio button is pressed, use up-down keys for volume
	  newDeviceMode = modeShowRadio_Volume;
	  if (radioIsOn)
	  {
	    if ((eventToHandle & BUTTON3_CLICK) || (longPressEvent.repPress & BUTTON3_CLICK))
	    {
	      SI4702_SetVolume(SI4702_GetVolume() - 1);
	      Renderer_Update_Secondary();
	    } else if ((eventToHandle & BUTTON4_CLICK) || (longPressEvent.repPress & BUTTON4_CLICK))
	    {
	      SI4702_SetVolume(SI4702_GetVolume() + 1);
	      Renderer_Update_Secondary();
	    }
	  }
	}
	break;
      }
      case modeAdjustYearTens:
      case modeAdjustYearOnes:
      case modeAdjustMonth:
      case modeAdjustDayTens:
      case modeAdjustDayOnes:
      case modeAdjustHoursTens:
      case modeAdjustHoursOnes:
      case modeAdjustMinsTens:
      case modeAdjustMinsOnes:
      {
	if (longPressEvent.longPress & BUTTON1_CLICK)
	{
	  // Abort setting time
	  newDeviceMode = modeShowTime;
	} else if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  if (deviceMode < modeAdjustMinsOnes)
	  {
	    newDeviceMode++;
	  }
	  else
	  {
	    // Done setting time
	    Write_DS1307_DateTime();
	    timePollAllowed = 1;
	    newDeviceMode = modeShowTime;
	  }
	} else if (eventToHandle & BUTTON3_CLICK)
	{
	  updateScreen = 1;
	  HandleEditDown(editMode, editDigit, editMaxValue);
	  if (deviceMode < modeAdjustHoursTens)
	  {
	    UpdateDOW();   
	  }
	} else if (eventToHandle & BUTTON4_CLICK)
	{
	  updateScreen = 1;
	  HandleEditUp(editMode, editDigit, editMaxValue);
	  if (deviceMode < modeAdjustHoursTens)
	  {
	    UpdateDOW();   
	  }
	}
	break;
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
        case modeShowTime:
	  Renderer_SetFlashMask(0);
	  editMode = 0;	  
	  secMode = SECONDARY_MODE_SEC;
	  mainMode = MAIN_MODE_TIME;
          break;
	case modeShowDate:
	  Renderer_SetFlashMask(0);
	  editMode = 0;
	  mainMode = MAIN_MODE_DATE;
	  secMode = SECONDARY_MODE_YEAR;
	  break;
	case modeShowRadio:
	  mainMode = MAIN_MODE_TIME;
	  secMode = SECONDARY_MODE_RADIO;
	  Renderer_Update_Secondary();
	  break;
	case modeShowRadio_Volume:
	  mainMode = MAIN_MODE_TIME;
	  secMode = SECONDARY_MODE_VOLUME;
	  Renderer_Update_Secondary();
	  break;
        default:
          break;
      }

      updateScreen = 1;

      deviceMode = newDeviceMode;
    }

    if (updateScreen)
    {
      // Something happened, update display.
      Renderer_Update_Main(mainMode, (eventToHandle & CLOCK_UPDATE) && (deviceMode == modeShowTime || deviceMode == modeShowRadio) );
    }

    if (eventToHandle == 0)
    {
      // Nothing to do, go to sleep
      if (beepIsOn)
	set_sleep_mode(SLEEP_MODE_IDLE); // keep timer 0 running!
      else
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
