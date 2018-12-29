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
#include <avr/io.h>
#include <util/delay.h>
#include "Renderer.h"
#include "Panels.h"
#include "DateTime.h"
#include "events.h"
#include "longpress.h"
#include "DS1307.h"
#include "SI4702.h"
#include "settings.h"
#include "Timefuncs.h"
#include "BCDFuncs.h"

#include "i2c.h"

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/atomic.h>

struct DateTime TheDateTime;
struct DateTime ThePreviousDateTime;

uint8_t TheSleepTime = 0;
uint8_t TheNapTime = 0;


#define BEEP_MARK             3
#define BEEP_SPACE           4
#define BEEP_COUNT           4
#define BEEP_ON_PERIOD       (BEEP_COUNT * (BEEP_MARK + BEEP_SPACE))
#define BEEP_PAUSE             BEEP_ON_PERIOD
#define BEEP_TOTALPERIOD     (BEEP_PAUSE + BEEP_ON_PERIOD)

#define SHOW_ALARM_TIMEOUT   15
#define TIME_ADJUST_TIMEOUT 15

#define ALARM_BEEP_TIMEOUT   4
#define ALARM_RADIO_TIMEOUT  60
#define INITIAL_SLEEPTIME    15
#define INITIAL_NAPTIME      INITIAL_SLEEPTIME

uint8_t beepState = 0;

_Bool radioIsOn = 0;
_Bool beepIsOn = 0;

enum clockMode
{
  modeShowTime,
  modeShowDate,
  modeShowRadio,
  modeShowRadio_Volume,
  modeShowAlarm1,
  modeShowAlarm2,
  modeShowOnetimeAlarm,
  modeAlarmFiring_beep,
  modeAlarmFiring_radio,
  modeAdjustYearTens,
  modeAdjustYearOnes,
  modeAdjustMonth,
  modeAdjustDayTens,
  modeAdjustDayOnes,
  modeAdjustHoursTens,
  modeAdjustHoursOnes,
  modeAdjustMinsTens,
  modeAdjustMinsOnes,
  modeAdjustHoursTens_Alarm,
  modeAdjustHoursOnes_Alarm,
  modeAdjustMinsTens_Alarm,
  modeAdjustMinsOnes_Alarm,
  modeAdjustDays_Alarm,
  modeAdjustType_Alarm,
  modeAdjustSleep,
  modeAdjustNap,
  modeAdjustTimeAdjust
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

static inline _Bool IsPastAlarmTime(const struct AlarmSetting *alarm, const struct DateTime *timestamp)
{
  return (timestamp->hour > alarm->hour) || (timestamp->hour == alarm->hour && timestamp->min >= alarm->min);
}

enum enumAlarmScheduleState {
  NOT_SCHEDULED = LED_OFF,          // Alarm is not scheduled for the next 24 hours
  SCHEDULED = LED_ON,                 // Alarm is scheduled
  SUSPENDED = LED_BLINK_SHORT,      // Alarm would've been scheduled, but the next invocation is suspended
};

enum enumAlarmScheduleState IsAlarmScheduled(struct AlarmSetting *alarm)
{
  if ((alarm->flags & ALARM_ACTIVE) == 0)
    return NOT_SCHEDULED;
  
  uint8_t dayBits = 0xff; // bit 0 = monday, bit 1 = tuesday ... bit 7 is monday again, to handle wraparound.
  
  switch (alarm->flags & ALARM_DAY_BITS)
  {
    case ALARM_DAY_WEEK:
      dayBits = 0x9f; // Trigger on mon-fri
      break;
    case ALARM_DAY_WEEKEND:
      dayBits = 0x60; // Trigger on sat-sun
  }
  
  // See if next alarm time is today or tomorrow.
  uint8_t nextAlarmDay; // 0-based
  
  if (IsPastAlarmTime(alarm, &TheDateTime))
  {
    // Alarm will trigger tomorrow.
    nextAlarmDay = TheDateTime.wday;
  }
  else
  {
    // Alarm will trigger today
    nextAlarmDay = TheDateTime.wday - 1; // 0-6
  }
    
  // Check if day matches
  if (! ( dayBits & (1 << nextAlarmDay)))
    return NOT_SCHEDULED;
    
  if ((alarm->flags & ALARM_SUSPENDED) == 0)
    return SCHEDULED;
  else
    return SUSPENDED;
}

void BeepOff()
{
  TCCR0B = 0; // Stop timer
  beepIsOn = 0;
  
  // Force beeper output low
  PORTC &= ~ _BV(PORTC1);
  
  // Amplifier control is active low
  PORTC = PORTC | _BV(PORTC2); 
}

void RadioOff()
{
  // Amplifier control is active low
  PORTC = PORTC | _BV(PORTC2);
  
  SI4702_PowerOff();
  radioIsOn = 0;
  TheSleepTime = 0;
}

void BeepOn()
{
  if (radioIsOn)
  {
    RadioOff();
  }
  
  beepIsOn = 1;
  beepState = BEEP_ON_PERIOD;

  // Amplifier control is active low
  PORTC = PORTC & ~( _BV(PORTC2)); 
}

_Bool RadioOn()
{
  if (radioIsOn)
    return 1; // Already on.
    
  if (beepIsOn)
  {
    BeepOff();
  }
  
  if (!SI4702_PowerOn())
  {
    return 0; // Failed to start the radio
  }
  
  SI4702_SetFrequency(TheGlobalSettings.radio.frequency);
  SI4702_SetVolume(TheGlobalSettings.radio.volume);
  
  Poll_SI4702();
  // Amplifier control is active low
  
  PORTC = PORTC & ~( _BV(PORTC2)); 
  radioIsOn = 1;  
  return 1;
}

uint8_t alarm1Timeout = 0; // minutes
uint8_t alarm2Timeout = 0; // minutes
uint8_t onetimeAlarmTimeout = 0; // minutes
uint8_t napTimeout = 0; // minutes

enum enumAlarmScheduleState alarm1Scheduled = NOT_SCHEDULED, alarm2Scheduled = NOT_SCHEDULED, onetimeAlarmScheduled = NOT_SCHEDULED;

void SilenceAlarm(struct AlarmSetting *alarm)
{
  if (alarm->flags & ALARM_TYPE_RADIO)
  {
    RadioOff();
  }
  else
  {
    BeepOff();
  }
}
  
_Bool AlarmTriggered(struct AlarmSetting *alarm)
{
  if (IsPastAlarmTime(alarm, &ThePreviousDateTime))
    return 0; // already triggered

  if (!IsPastAlarmTime(alarm, &TheDateTime) && ThePreviousDateTime.hour <= TheDateTime.hour)
    return 0; // Alarm not yet triggered, and no roll-over

  if ((alarm->flags & ALARM_DAY_NEVER) == ALARM_DAY_NEVER)
  {
    // Turn off alarm on one-time alarms
    alarm->flags &= ~ALARM_ACTIVE;
  }
  
  return 1;
}

void HandleCycleTime(uint8_t *time)
{
  uint8_t remainder = *time % 15;
  
  *time += (15 - remainder);
  if (*time > 150)
    *time = 0;
}

struct deviceState
{
  uint8_t modeTimeout;
  int8_t  timeAdjustRemainder;
  _Bool   timeAdjustApplied;
  enum clockMode deviceMode;

} TheDeviceState;

void SilenceAlarms()
{
  if (alarm1Timeout)
  {
    alarm1Timeout--;
    if (!alarm1Timeout)
      SilenceAlarm(&TheGlobalSettings.alarm1);
  }
  
  if (alarm2Timeout)
  {
    alarm2Timeout--;
    if (!alarm2Timeout)
      SilenceAlarm(&TheGlobalSettings.alarm2);
  }
  
  if (onetimeAlarmTimeout)
  {
    onetimeAlarmTimeout--;
    if (!onetimeAlarmTimeout)
      SilenceAlarm(&TheGlobalSettings.onetime_alarm);
  }
  
  if (TheSleepTime && TheDeviceState.deviceMode != modeAdjustSleep)
  {
    TheSleepTime--;
    if (TheSleepTime == 0)
    {
      RadioOff();
      if (!TheDeviceState.modeTimeout)
        TheDeviceState.modeTimeout = 1;
    }
  }
  
  if (napTimeout)
  {
    napTimeout--;
    if (!napTimeout)
    {
      BeepOff();
    }
  }
}

enum clockMode ActivateAlarms()
{
  enum clockMode newDeviceMode = TheDeviceState.deviceMode;

  if (TheNapTime && TheDeviceState.deviceMode != modeAdjustNap)
  {
    TheNapTime--;
    if (TheNapTime == 0)
    {
      BeepOn();
      alarm1Timeout = 0;
      alarm2Timeout = 0;
      TheSleepTime = 0;
      napTimeout = ALARM_BEEP_TIMEOUT;
      newDeviceMode = modeAlarmFiring_beep;
    }
  }
  
  // See if alarms must fire.
  
  switch(alarm1Scheduled)
  {
    case NOT_SCHEDULED:
      break;
    case SCHEDULED:
      if (AlarmTriggered(&TheGlobalSettings.alarm1))
      {
        napTimeout = 0;
        alarm2Timeout = 0;
        onetimeAlarmTimeout = 0;
        TheSleepTime = 0;
        
        if ((TheGlobalSettings.alarm1.flags & ALARM_TYPE_RADIO) && RadioOn())
        {
          // Radio alarm, and radio could be started
          
          alarm1Timeout = ALARM_RADIO_TIMEOUT;
          newDeviceMode = modeAlarmFiring_radio;
        }
        else
        {
          // Beep alarm, or failed to start radio
          BeepOn();
          alarm1Timeout = ALARM_BEEP_TIMEOUT;
          newDeviceMode = modeAlarmFiring_beep;
        }
      }
      break;
    case SUSPENDED:
      if (AlarmTriggered(&TheGlobalSettings.alarm1))
      {
        // Remove suspend state
        TheGlobalSettings.alarm1.flags &= ~(ALARM_SUSPENDED);
      }
      break;
  }
  
  switch(alarm2Scheduled)
  {
    case NOT_SCHEDULED:
      break;
    case SCHEDULED:
      if (AlarmTriggered(&TheGlobalSettings.alarm2))
      {
        napTimeout = 0;
        alarm1Timeout = 0;
        onetimeAlarmTimeout = 0;
        TheSleepTime = 0;
        
        if ((TheGlobalSettings.alarm2.flags & ALARM_TYPE_RADIO) && RadioOn())
        {
          // Radio alarm, and radio could be started
          
          alarm2Timeout = ALARM_RADIO_TIMEOUT;
          newDeviceMode = modeAlarmFiring_radio;
        }
        else
        {
          // Beep alarm, or failed to start radio
          BeepOn();
          alarm2Timeout = ALARM_BEEP_TIMEOUT;
          newDeviceMode = modeAlarmFiring_beep;
        }
      }
      break;
    case SUSPENDED:
      if (AlarmTriggered(&TheGlobalSettings.alarm2))
      {
        // Remove suspend state
        TheGlobalSettings.alarm2.flags &= ~(ALARM_SUSPENDED);
      }
      break;
  }
  
  if(onetimeAlarmScheduled == SCHEDULED && AlarmTriggered(&TheGlobalSettings.onetime_alarm))
  {
    napTimeout = 0;
    alarm1Timeout = 0;
    alarm2Timeout = 0;
    TheSleepTime = 0;
    
    if ((TheGlobalSettings.onetime_alarm.flags & ALARM_TYPE_RADIO) && RadioOn())
    {
      // Radio alarm, and radio could be started
      
      onetimeAlarmTimeout = ALARM_RADIO_TIMEOUT;
      newDeviceMode = modeAlarmFiring_radio;
    }
    else
    {
      // Beep alarm, or failed to start radio
      BeepOn();
      onetimeAlarmTimeout = ALARM_BEEP_TIMEOUT;
      newDeviceMode = modeAlarmFiring_beep;
    }
  }

  return newDeviceMode;
}

void ApplyTimeAdjust()
{
  int8_t adjust = 0;

  TheDeviceState.timeAdjustRemainder += TheGlobalSettings.time_adjust;

  if (TheDeviceState.timeAdjustRemainder >= 10)
  {
    while(TheDeviceState.timeAdjustRemainder >= 10)
    {
      ++adjust;
      TheDeviceState.timeAdjustRemainder -= 10;
    }

    if (adjust >= 10)
      adjust += 6;

    TheDateTime.sec = BCDAdd(TheDateTime.sec, adjust);
    Write_DS1307_DateTime();
  }
  else if (TheDeviceState.timeAdjustRemainder <= -10)
  {
    while(TheDeviceState.timeAdjustRemainder <= -10)
    {
      ++adjust;
      TheDeviceState.timeAdjustRemainder += 10;
    }

    if (adjust >= 10)
      adjust += 6;

    TheDateTime.sec = BCDSub(TheDateTime.sec, adjust);
    Write_DS1307_DateTime();
  }
}

int main(void)
{
  // Setup watchdog
  wdt_enable(WDTO_4S);
  
  // Enable output on PORT C2 (amplifier control) and C1 (beeper)
  DDRC |= _BV(PORTC2) | _BV(PORTC1);
  // Amplifier control is active low
  PORTC = PORTC | _BV(PORTC2);
  
  wdt_reset();
  
  InitializePanels(4);
  
  Init_I2C();
  Init_DS1307();
  Init_SI4702();
  Renderer_Init();
  
  // Read radio defaults
  if (!ReadGlobalSettings())
  {
    // settings are lost, initialize
    TheGlobalSettings.radio.frequency = 875; // start of band
    TheGlobalSettings.radio.volume = 26;
    
    TheGlobalSettings.alarm1.flags = ALARM_TYPE_RADIO | ALARM_DAY_WEEK; // Disabled, Repeat on weekdays, radio
    TheGlobalSettings.alarm1.hour = 7;
    TheGlobalSettings.alarm1.min = 0;
    
    TheGlobalSettings.alarm2.flags = ALARM_DAY_WEEKEND; // Disabled, repeat on weekend, beeper
    TheGlobalSettings.alarm2.hour = 9;
    TheGlobalSettings.alarm2.min = 0x15; 
    
    TheGlobalSettings.onetime_alarm.flags = ALARM_DAY_NEVER; // Never repeat, disabled
    TheGlobalSettings.onetime_alarm.hour = 6;
    TheGlobalSettings.onetime_alarm.min = 0x45;
    
    TheGlobalSettings.brightness = 13;
    TheGlobalSettings.brightness_night = 3;
    TheGlobalSettings.time_adjust = 0; 
    WriteGlobalSettings();
  }

  SetBrightness(GetActiveBrightness(&TheDateTime));
  
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

  TheDeviceState.deviceMode = modeShowTime;
  TheDeviceState.modeTimeout = 0;
  TheDeviceState.timeAdjustApplied = true;
  TheDeviceState.timeAdjustRemainder = false;

  uint8_t *editDigit = 0;
  uint8_t editMode = 0;
  uint8_t editMaxValue = 0x99;
  
  uint8_t writeSettingTimeout = 0; // seconds
  
  _Bool timePollAllowed = 1;
  
  sei(); // Enable interrupts. This will immediately trigger a port change interrupt; sink these events.
  
  struct AlarmSetting alarmBeingModified;
  enum clockMode alarmAdjustReturnMode = 0;
  
  uint16_t clockEvents = 0;
  uint16_t buttonEvents = 0;
  struct longPressResult longPressEvent;
  
  longPressEvent.shortPress = 0;
  longPressEvent.longPress = 0;
  longPressEvent.repPress = 0;
  
  while (1)
  {
    
    wdt_reset();
  
    _Bool updateScreen = 0;
    uint16_t acceptedEvents = 0;
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      acceptedEvents = event;
      event = 0;
    }
    
    GetLongPress(acceptedEvents, &longPressEvent);
    clockEvents = (acceptedEvents & (CLOCK_UPDATE | CLOCK_TICK)) ;
    buttonEvents |= (acceptedEvents & ~(CLOCK_UPDATE | CLOCK_TICK));
      
    enum clockMode newDeviceMode = TheDeviceState.deviceMode;

    if (clockEvents & CLOCK_UPDATE)
    {
      if (writeSettingTimeout)
      {
        --writeSettingTimeout;
        if (writeSettingTimeout == 0)
        {
          WriteGlobalSettings();
        }
      }
      
      if (TheDeviceState.modeTimeout && --TheDeviceState.modeTimeout == 0)
      {
         newDeviceMode = (radioIsOn?modeShowRadio : modeShowTime) ;
      }
    
      if (timePollAllowed)
      {
        Read_DS1307_DateTime();
        
        updateScreen = 1;

        if (!TheDeviceState.timeAdjustApplied && TheDateTime.sec >= 30)
        {
          ApplyTimeAdjust();
          TheDeviceState.timeAdjustApplied = true;
        }        

        if (ThePreviousDateTime.sec > TheDateTime.sec)
        {
          // Second rollover.

          if (ThePreviousDateTime.min > TheDateTime.min && TheDateTime.hour == 0)
          {
            // Day rollover. Check for time drift at the 30 second mark.  
            TheDeviceState.timeAdjustApplied = false;
          }

          // See if alarms must timeout
          SilenceAlarms();

          newDeviceMode = ActivateAlarms();
          
          SetBrightness(GetActiveBrightness(&TheDateTime));
        }        

        ThePreviousDateTime = TheDateTime;
        
        if (TheDeviceState.deviceMode < modeShowAlarm1)
        {
          alarm1Scheduled = IsAlarmScheduled(&TheGlobalSettings.alarm1);
          alarm2Scheduled = IsAlarmScheduled(&TheGlobalSettings.alarm2);
          onetimeAlarmScheduled = IsAlarmScheduled(&TheGlobalSettings.onetime_alarm);
          
          Renderer_SetLed((TheNapTime + TheSleepTime) > 0 ? LED_ON : LED_OFF, alarm1Scheduled,  alarm2Scheduled, onetimeAlarmScheduled);
        }
      }
    }
    
    if (newDeviceMode == TheDeviceState.deviceMode)
    {
      // No timer-related changes, probe the keys
      
      // Handle keypresses
      switch ( TheDeviceState.deviceMode )
      {
        case modeShowTime:
        {
          if (buttonEvents & BUTTON5_CLICK)
          {
            if (radioIsOn)
            {
              newDeviceMode = modeAdjustSleep;
            }
            else
            {
              newDeviceMode = modeAdjustNap;
              
            }
            break;
          }
          if (buttonEvents & BUTTON2_CLICK)
          {
            MarkLongPressHandled(BUTTON2_CLICK);
            if (radioIsOn)
            {
              RadioOff();
            }
            else
            {
              if (RadioOn())
                newDeviceMode = modeShowRadio;
            }
            break;
          }
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // adjust time
            newDeviceMode = modeAdjustYearTens;
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode = modeShowDate;
          } else if (longPressEvent.repPress & BUTTON3_CLICK)
          {
            SetBrightness(DecreaseBrightness(&TheDateTime));
            writeSettingTimeout = 5;
          } else if (longPressEvent.repPress & BUTTON4_CLICK)
          {
            SetBrightness(IncreaseBrightness(&TheDateTime));
            writeSettingTimeout = 5;
          }
          break;
        }
        case modeShowDate:
        {
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // adjust time adjust
            newDeviceMode = modeAdjustTimeAdjust;
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode = modeShowAlarm1;
          }
          break;
        }
        case modeShowRadio:
          if (buttonEvents & BUTTON5_CLICK)
          {        
            newDeviceMode = modeAdjustSleep;
            break;
          }
          // fall-through
        case modeShowRadio_Volume:
        {
          
          if (longPressEvent.shortPress & BUTTON2_CLICK)
          {
            RadioOff();
            newDeviceMode = modeShowTime;
            break;
          }
          
          if (buttonEvents & BUTTON1_CLICK)
          {
            MarkLongPressHandled(BUTTON1_CLICK);
            newDeviceMode = modeShowAlarm1;
            break;
          } 
          
          if (PIND & BUTTON2_CLICK) 
          {
            // Radio button is not pressed, use up-down keys for tuning
            newDeviceMode = modeShowRadio;
            if (radioIsOn)
            {
              if (buttonEvents & BUTTON3_CLICK)
              {
                writeSettingTimeout = 0; // Postpone writing settings, the SI4702 prefers the I2C bus t be quiet
                SI4702_Tune(0);
              } else if (buttonEvents & BUTTON4_CLICK)
              {
                writeSettingTimeout = 0; // Postpone writing settings, the SI4702 prefers the I2C bus t be quiet
                SI4702_Tune(1);
              } else if (longPressEvent.longPress & BUTTON3_CLICK)
              {
                writeSettingTimeout = 0; // Postpone writing settings, the SI4702 prefers the I2C bus t be quiet
                SI4702_Seek(0);
              } else if (longPressEvent.longPress & BUTTON4_CLICK)
              {
                writeSettingTimeout = 0; // Postpone writing settings, the SI4702 prefers the I2C bus t be quiet
                SI4702_Seek(1);
              }
            }
          } else
          {
            // Radio button is pressed, use up-down keys for volume
            newDeviceMode = modeShowRadio_Volume;
            if (radioIsOn)
            {
              if ((buttonEvents & BUTTON3_CLICK) || (longPressEvent.repPress & BUTTON3_CLICK))
              {
                if (TheGlobalSettings.radio.volume > 1)
                {
                  TheGlobalSettings.radio.volume--;
                  SI4702_SetVolume(TheGlobalSettings.radio.volume);
                  writeSettingTimeout = 5;
                  Renderer_Update_Secondary();
                }
              } else if ((buttonEvents & BUTTON4_CLICK) || (longPressEvent.repPress & BUTTON4_CLICK))
              {
                if (TheGlobalSettings.radio.volume < 30)
                {
                  TheGlobalSettings.radio.volume++;
                  SI4702_SetVolume(TheGlobalSettings.radio.volume);
                  writeSettingTimeout = 5;
                  Renderer_Update_Secondary();
                }
              }
            }
          }
          break;
        }
        
        case modeShowAlarm1:
          if ( buttonEvents & BUTTON2_CLICK )
          {
            TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
            TheGlobalSettings.alarm1.flags ^= ALARM_ACTIVE;
            Renderer_SetLed(((TheNapTime + TheSleepTime) > 0) ? LED_ON : LED_OFF, (TheGlobalSettings.alarm1.flags & ALARM_ACTIVE) ? LED_BLINK_LONG : LED_BLINK_SHORT, alarm2Scheduled, onetimeAlarmScheduled );        
          }
          
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // adjust alarm
            alarmBeingModified = TheGlobalSettings.alarm1;
            alarmAdjustReturnMode = modeShowAlarm1;
            newDeviceMode = modeAdjustHoursTens_Alarm;
            
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode = modeShowAlarm2;
          }
          
          if (alarm1Scheduled != NOT_SCHEDULED)
          {
            if (buttonEvents & (BUTTON3_CLICK | BUTTON4_CLICK))
            {
              // Toggle alarm suspend
              TheGlobalSettings.alarm1.flags ^= ALARM_SUSPENDED;
              TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
              Renderer_Update_Secondary();
            }
          }
          break;
          
        case modeShowAlarm2:
          if ( buttonEvents & BUTTON2_CLICK )
          {
            TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
            TheGlobalSettings.alarm2.flags ^= ALARM_ACTIVE;
            Renderer_SetLed( ((TheNapTime + TheSleepTime) > 0 )? LED_ON : LED_OFF, alarm1Scheduled, (TheGlobalSettings.alarm2.flags & ALARM_ACTIVE )? LED_BLINK_LONG : LED_BLINK_SHORT, onetimeAlarmScheduled);
          }
          
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // adjust alarm
            alarmBeingModified = TheGlobalSettings.alarm2;
            alarmAdjustReturnMode = modeShowAlarm2;
            newDeviceMode = modeAdjustHoursTens_Alarm;
            
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode = modeShowOnetimeAlarm;
          }
          
          if (alarm2Scheduled != NOT_SCHEDULED)
          {
            if (buttonEvents & (BUTTON3_CLICK | BUTTON4_CLICK))
            {
              // Toggle alarm suspend
              TheGlobalSettings.alarm2.flags ^= ALARM_SUSPENDED;
              TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
              Renderer_Update_Secondary();
            }
          }
          break;
        case modeShowOnetimeAlarm:
          if ( buttonEvents & BUTTON2_CLICK )
          {
            TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
            TheGlobalSettings.onetime_alarm.flags ^= ALARM_ACTIVE;
            Renderer_SetLed( ((TheNapTime + TheSleepTime) > 0 )? LED_ON : LED_OFF, alarm1Scheduled, alarm2Scheduled, (TheGlobalSettings.onetime_alarm.flags & ALARM_ACTIVE )? LED_BLINK_LONG : LED_BLINK_SHORT);
          }
          
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // adjust alarm
            alarmBeingModified = TheGlobalSettings.onetime_alarm;
            alarmAdjustReturnMode = modeShowOnetimeAlarm;
            newDeviceMode = modeAdjustHoursTens_Alarm;
            
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            if (radioIsOn)
            {
              newDeviceMode = modeShowRadio;
            }
            else
              newDeviceMode = modeShowTime;
          }
          break;
        case modeAlarmFiring_beep:
        case modeAlarmFiring_radio:
          {
            if (buttonEvents & (BUTTON1_CLICK | BUTTON2_CLICK | BUTTON3_CLICK | BUTTON4_CLICK | BUTTON5_CLICK))
            {
              if (napTimeout)
              {
                napTimeout = 0; 
                BeepOff();
              }
              else
              {
                // Silence all alarms
                if (alarm1Timeout)
                {
                  SilenceAlarm(&TheGlobalSettings.alarm1);
                  alarm1Timeout = 0;
                }
                
                if (alarm2Timeout)
                {
                  SilenceAlarm(&TheGlobalSettings.alarm2);
                  alarm2Timeout = 0;
                }
                
                if (onetimeAlarmTimeout)
                {
                  SilenceAlarm(&TheGlobalSettings.onetime_alarm);
                  onetimeAlarmTimeout = 0;
                }

              }
            }
            if (!alarm1Timeout && !alarm2Timeout && !napTimeout && !onetimeAlarmTimeout) 
            {
              MarkLongPressHandled(buttonEvents); // don't generate shortpresses. 
              newDeviceMode = modeShowTime;
            }
          }
          break;
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
            if (TheDeviceState.deviceMode < modeAdjustMinsOnes)
            {
              newDeviceMode++;
            }
            else
            {
              // Done setting time
              Write_DS1307_DateTime();
              newDeviceMode = modeShowTime;
            }
          } else if (buttonEvents & BUTTON3_CLICK)
          {
            updateScreen = 1;
            HandleEditDown(editMode, editDigit, editMaxValue);
            if (TheDeviceState.deviceMode < modeAdjustHoursTens)
            {
              TheDateTime.wday = GetDayOfWeek(TheDateTime.day, TheDateTime.month, TheDateTime.year /* 20xx */);
            }
          } else if (buttonEvents & BUTTON4_CLICK)
          {
            updateScreen = 1;
            HandleEditUp(editMode, editDigit, editMaxValue);
            if (TheDeviceState.deviceMode < modeAdjustHoursTens)
            {
               TheDateTime.wday = GetDayOfWeek(TheDateTime.day, TheDateTime.month, TheDateTime.year /* 20xx */);
            }
          }
          break;
        }
        case modeAdjustHoursTens_Alarm:
        case modeAdjustHoursOnes_Alarm:
        case modeAdjustMinsTens_Alarm:
        case modeAdjustMinsOnes_Alarm:
        {
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // Abort adjusting alarm
            newDeviceMode = alarmAdjustReturnMode;
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode++;
            if (newDeviceMode == modeAdjustDays_Alarm && alarmAdjustReturnMode == modeShowOnetimeAlarm)
            {
              newDeviceMode = modeAdjustType_Alarm; // Skip day selection for one-time alarm.
            }
          }
          else if (buttonEvents & BUTTON3_CLICK)
          {
            updateScreen = 1;
            HandleEditDown(editMode, editDigit, editMaxValue);
          }
          else if (buttonEvents & BUTTON4_CLICK)
          {
            updateScreen = 1;
            HandleEditUp(editMode, editDigit, editMaxValue);
          }
          break;        
        }
        case modeAdjustDays_Alarm:
        {
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // Abort adjusting alarm
            newDeviceMode = alarmAdjustReturnMode;
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode++;
          }
          else if (buttonEvents & BUTTON3_CLICK)
          {
            updateScreen = 1;
            uint8_t newDays = (alarmBeingModified.flags - 4 ) & ALARM_DAY_BITS;
            if (newDays == ALARM_DAY_NEVER)
              newDays = ALARM_DAY_WEEKEND;
              
            alarmBeingModified.flags &= ~ALARM_DAY_BITS;
            alarmBeingModified.flags |= newDays;
          }
          else if (buttonEvents & BUTTON4_CLICK)
          {
            updateScreen = 1;
            uint8_t newDays = (alarmBeingModified.flags + 4 ) & ALARM_DAY_BITS;
               if (newDays == ALARM_DAY_NEVER)
              newDays = ALARM_DAY_DAILY;

            alarmBeingModified.flags &= ~ALARM_DAY_BITS;
            alarmBeingModified.flags |= newDays;
          }
          break;
        }
        case modeAdjustType_Alarm:
        {
          if (longPressEvent.longPress & BUTTON1_CLICK)
          {
            // Abort adjusting alarm
            newDeviceMode = alarmAdjustReturnMode;
          } else if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            switch(alarmAdjustReturnMode)
            {
              case modeShowAlarm1:
                TheGlobalSettings.alarm1 = alarmBeingModified;
                break;
              case modeShowAlarm2:
                TheGlobalSettings.alarm2 = alarmBeingModified;
                break;
              case modeShowOnetimeAlarm:
                TheGlobalSettings.onetime_alarm = alarmBeingModified;
              default:
                break;              
            }
           
            newDeviceMode = alarmAdjustReturnMode;
              
            MarkLongPressHandled(BUTTON1_CLICK);
            writeSettingTimeout = 5;
          }
          else if (buttonEvents & (BUTTON3_CLICK | BUTTON4_CLICK))
          {
            Renderer_Update_Secondary();
            alarmBeingModified.flags ^= ALARM_TYPE_RADIO;
          }
          break;
        }
        
        case modeAdjustSleep:
        {
          if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode = modeShowRadio;
          } else if (buttonEvents & BUTTON5_CLICK)
          {
            TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT; 
            HandleCycleTime(&TheSleepTime);
            updateScreen = 1;
          }
          break;      
        }
        case modeAdjustNap:
        {
          if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode = modeShowTime;
          } else if (buttonEvents & BUTTON5_CLICK)
          {
            TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT; 
            HandleCycleTime(&TheNapTime);
            updateScreen = 1;
          }
          break;      
        }
        case modeAdjustTimeAdjust:
        {
          if (longPressEvent.shortPress & BUTTON1_CLICK)
          {
            newDeviceMode = modeShowDate;
          }else if(longPressEvent.shortPress & BUTTON4_CLICK)
          {
            if (TheGlobalSettings.time_adjust < 99)
            {
              TheGlobalSettings.time_adjust++;
              TheDeviceState.modeTimeout = TIME_ADJUST_TIMEOUT;
              updateScreen = 1;
              writeSettingTimeout = TIME_ADJUST_TIMEOUT;
            }
          } else if (longPressEvent.shortPress & BUTTON3_CLICK)
          {
            if (TheGlobalSettings.time_adjust > -99)
            {
              TheGlobalSettings.time_adjust--;
              updateScreen = 1;
              writeSettingTimeout = TIME_ADJUST_TIMEOUT;
            }
          }
          break;
        }
      }
   
      // Mark all button events as handled
      
      buttonEvents = 0;
      longPressEvent.shortPress = 0;
      longPressEvent.longPress = 0;
      longPressEvent.repPress = 0;
    }      
    
    if (newDeviceMode != TheDeviceState.deviceMode)
    {
      // Update the screen.
      switch(newDeviceMode)
      {
        case modeAdjustYearTens:
          TheDeviceState.modeTimeout = 255;
          mainMode = MAIN_MODE_DATE;
          secMode = SECONDARY_MODE_YEAR;
          timePollAllowed = 0;
          
          Renderer_SetFlashMask(0x2); 
          editDigit = &TheDateTime.year;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x99;
          break;
        case modeAdjustYearOnes:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x1); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustMonth:
          TheDeviceState.modeTimeout = 255;
          editMode = EDIT_MODE_ONES | EDIT_MODE_ONEBASE; 
          editDigit = &TheDateTime.month;
          editMaxValue = 0x12;
          Renderer_SetFlashMask(0x30); 
          break;
        case modeAdjustDayTens:
          TheDeviceState.modeTimeout = 255;
          editDigit = &TheDateTime.day;
          editMaxValue = GetDaysPerMonth(TheDateTime.month, TheDateTime.year);
          editMode = EDIT_MODE_TENS | EDIT_MODE_ONEBASE;
          Renderer_SetFlashMask(0x80); 
          break;
        case modeAdjustDayOnes:
          TheDeviceState.modeTimeout = 255;
          editMode = EDIT_MODE_ONES | EDIT_MODE_ONEBASE;
          Renderer_SetFlashMask(0x40); 
          break;
        case modeAdjustHoursTens:
          TheDeviceState.modeTimeout = 255;
          mainMode = MAIN_MODE_TIME;
          secMode = SECONDARY_MODE_SEC;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x23;
          editDigit = &TheDateTime.hour;
          Renderer_SetFlashMask(0x80); 
          break;
        case modeAdjustHoursOnes:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x40); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustMinsTens:
          TheDeviceState.modeTimeout = 255;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x59;
          editDigit = &TheDateTime.min;
          Renderer_SetFlashMask(0x20); 
          break;
        case modeAdjustMinsOnes:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x10); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeShowTime:
          TheDeviceState.modeTimeout = 0;
          Renderer_SetInverted(NOT_INVERTED);
          Renderer_SetFlashMask(0);
          timePollAllowed = 1;
          editMode = 0;          
          secMode = SECONDARY_MODE_SEC;
          mainMode = MAIN_MODE_TIME;
          break;
        case modeShowDate:
          TheDeviceState.modeTimeout = 3;
          Renderer_SetFlashMask(0);
          editMode = 0;
          mainMode = MAIN_MODE_DATE;
          secMode = SECONDARY_MODE_YEAR;
          break;
        case modeShowRadio:
          timePollAllowed = 1;
          TheDeviceState.modeTimeout = 0;
          mainMode = MAIN_MODE_TIME;
          secMode = SECONDARY_MODE_RADIO;
          Renderer_Update_Secondary();
          break;
        case modeShowRadio_Volume:
          timePollAllowed = 1;
          TheDeviceState.modeTimeout = 0;
          mainMode = MAIN_MODE_TIME;
          secMode = SECONDARY_MODE_VOLUME;
          Renderer_Update_Secondary();
          break;
        case modeAlarmFiring_beep:
          TheDeviceState.modeTimeout = 0;
          Renderer_SetInverted(NOT_INVERTED);
          Renderer_SetFlashMask(0xff);
          timePollAllowed = 1;
          editMode = 0;          
          secMode = SECONDARY_MODE_SEC;
          mainMode = MAIN_MODE_TIME;
          break;
       case modeAlarmFiring_radio:
          TheDeviceState.modeTimeout = 0;
          Renderer_SetInverted(INVERTED);
          Renderer_SetFlashMask(0x0);
          timePollAllowed = 1;
          editMode = 0;          
          secMode = SECONDARY_MODE_RADIO;
          mainMode = MAIN_MODE_TIME;
          break;
        case modeShowAlarm1:
        {
          Renderer_SetFlashMask(0x00); 
          timePollAllowed = 1;
          TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
          mainMode = MAIN_MODE_ALARM;
          secMode = SECONDARY_MODE_ALARM;
          Renderer_SetAlarmStruct(&TheGlobalSettings.alarm1);
          Renderer_SetLed((TheNapTime + TheSleepTime > 0 ) ? LED_ON : LED_OFF, (TheGlobalSettings.alarm1.flags & ALARM_ACTIVE) ? LED_BLINK_LONG : LED_BLINK_SHORT, alarm2Scheduled, onetimeAlarmScheduled);
          Renderer_Update_Secondary();
          break;
        }
        case modeShowAlarm2:
          Renderer_SetFlashMask(0x00); 
          timePollAllowed = 1;
          TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
          mainMode = MAIN_MODE_ALARM;
          secMode = SECONDARY_MODE_ALARM;
          Renderer_SetAlarmStruct(&TheGlobalSettings.alarm2);
          Renderer_SetLed((TheNapTime + TheSleepTime > 0 ) ? LED_ON : LED_OFF, alarm1Scheduled, (TheGlobalSettings.alarm2.flags & ALARM_ACTIVE) ? LED_BLINK_LONG : LED_BLINK_SHORT, onetimeAlarmScheduled);
          Renderer_Update_Secondary();
          break;
        case modeShowOnetimeAlarm:
          Renderer_SetFlashMask(0x00); 
          timePollAllowed = 1;
          TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT;
          mainMode = MAIN_MODE_ALARM;
          secMode = SECONDARY_MODE_ALARM;
          Renderer_SetAlarmStruct(&TheGlobalSettings.onetime_alarm);
          Renderer_SetLed((TheNapTime + TheSleepTime > 0 ) ? LED_ON : LED_OFF, alarm1Scheduled, alarm2Scheduled, (TheGlobalSettings.onetime_alarm.flags & ALARM_ACTIVE) ? LED_BLINK_LONG : LED_BLINK_SHORT);
          Renderer_Update_Secondary();
          break;
        case modeAdjustHoursTens_Alarm:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x80); 
          Renderer_SetAlarmStruct(&alarmBeingModified);
          editDigit = &alarmBeingModified.hour;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x23;
          break;
        case modeAdjustHoursOnes_Alarm:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x40); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustMinsTens_Alarm:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x20); 
          editDigit = &alarmBeingModified.min;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x59;
          break;
        case modeAdjustMinsOnes_Alarm:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x10); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustDays_Alarm:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x100); 
          editMode = 0;
          break;
        case modeAdjustType_Alarm:
          TheDeviceState.modeTimeout = 255;
          Renderer_SetFlashMask(0x0f);
          break;
        case modeAdjustNap: 
          if (TheNapTime == 0)
            TheNapTime = INITIAL_NAPTIME;
          TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT; 
          mainMode = MAIN_MODE_NAP;
          secMode = SECONDARY_MODE_NAP;
          Renderer_SetLed(LED_BLINK_SHORT, alarm1Scheduled,  alarm2Scheduled, onetimeAlarmScheduled);
          break;
        case modeAdjustSleep:
          if (TheSleepTime == 0)
            TheSleepTime = INITIAL_SLEEPTIME;
          TheDeviceState.modeTimeout = SHOW_ALARM_TIMEOUT; 
          mainMode = MAIN_MODE_SLEEP;
          secMode = SECONDARY_MODE_SLEEP;
          Renderer_SetLed(LED_BLINK_SHORT, alarm1Scheduled,  alarm2Scheduled, onetimeAlarmScheduled);
          break;
        case modeAdjustTimeAdjust:
          TheDeviceState.modeTimeout = TIME_ADJUST_TIMEOUT;
          Renderer_SetFlashMask(0);
          editMode = 0;
          mainMode = MAIN_MODE_DATE;
          secMode = SECONDARY_MODE_TIME_ADJUST;
          break;
        default:
        
          break;
      }

      updateScreen = 1;

      TheDeviceState.deviceMode = newDeviceMode;
    }

    if (updateScreen)
    {
      // Something happened, update display.
      Renderer_Update_Main(mainMode, (clockEvents & CLOCK_UPDATE) && (TheDeviceState.deviceMode == modeShowTime ) );
    }

    if (clockEvents & CLOCK_TICK)
    {
      if (radioIsOn && Poll_SI4702() && TheDeviceState.deviceMode == modeShowRadio)
      {
        // Radio is done seeking or tuning
        Renderer_Update_Secondary();
        TheGlobalSettings.radio.frequency = SI4702_GetFrequency();
        writeSettingTimeout = 5;
      }
      
      Renderer_Tick(secMode);
    }
   
    if (clockEvents == 0 && buttonEvents == 0)
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
