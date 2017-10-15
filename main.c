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

#include "i2c.h"

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>

struct DateTime TheDateTime;
struct DateTime ThePreviousDateTime;

uint8_t TheSleepTime = 0;
uint8_t TheNapTime = 0;

#define EDIT_MODE_ONES     0x1
#define EDIT_MODE_TENS     0x2
#define EDIT_MODE_NUMBER   (EDIT_MODE_ONES |EDIT_MODE_TENS)
#define EDIT_MODE_MASK     0x03
#define EDIT_MODE_ONEBASE 0x04

#define BEEP_MARK	     3
#define BEEP_SPACE           4
#define BEEP_COUNT           4
#define BEEP_ON_PERIOD       (BEEP_COUNT * (BEEP_MARK + BEEP_SPACE))
#define BEEP_PAUSE	     BEEP_ON_PERIOD
#define BEEP_TOTALPERIOD     (BEEP_PAUSE + BEEP_ON_PERIOD)

#define SHOW_ALARM_TIMEOUT   15

#define ALARM_BEEP_TIMEOUT   4
#define ALARM_RADIO_TIMEOUT  60
#define INITIAL_SLEEPTIME    2
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
  modeAlarmFiring,
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

uint8_t BCDToDec(const uint8_t bcd)
{
  return 10 * (bcd >> 4) + (bcd & 0x0f);
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
  uint8_t days = pgm_read_byte(dpm + BCDToDec(TheDateTime.month) -1);
  const uint8_t year = BCDToDec(TheDateTime.year);
  if (TheDateTime.month == 2 && (year %4 == 0))
    days++; // February on a leap year
  
  return days;
}

const uint8_t PROGMEM dowTable[] = { 0,3,2,5,0,3,5,1,4,6,2,4 };

void UpdateDOW()
{
  uint8_t year = BCDToDec(TheDateTime.year);
  const uint8_t month = BCDToDec(TheDateTime.month);
  const uint8_t day = BCDToDec(TheDateTime.day) - 1;
  
  year -= ( month < 3);
  
  TheDateTime.wday = ((year + year / 4 /* -y/100 + y / 400 */ + pgm_read_byte(dowTable + month -1) + day) % 7) + 1;
}

_Bool DetermineDST()
{  
  // Initialize DST. 
  _Bool dstActive;
  
  if (TheDateTime.month < 3 || TheDateTime.month > 0x10 || (TheDateTime.month == 3 && TheDateTime.day < 0x25))
  {
    // Before march 25th (Earliest possible date of DST changeover) or past october: DST not active
    dstActive = 0;
  }
  else if ((TheDateTime.month > 3 && TheDateTime.month < 0x10) || (TheDateTime.month == 0x10 && TheDateTime.day < 0x25))
  {
    // Between April 1st and october 25th (Earliest possible date of DST changeover): DST active
    dstActive = 1;
  }
  else 
  {
    // It is march, and the day is >= 25, and there are 31 days in march. That,
    // or it is october and the day is >=25, and there are 31 days in october.
    
    // Switchover is on the last sunday of march or the last sunday of october:
    // 
    // This means there are 7 possibilities:
    //
    //  Mon Tue Wed Thu Fri Sat Sun Mon Tue Wed Thu Fri Sat       dow_25
    //
    //   25  26  27  28  29  30 *31*                                1
    //       25  26  27  28  29 *30* 31                             2 
    //           25  26  27  28 *29* 30  31                         3
    //               25  26  27 *28* 29  30  31                     4
    //                   25  26 *27* 28  29  30  31                 5
    //                       25 *26* 27  28  29  30  31             6 
    //                          *25* 26  27  28  29  30  31         7
    
    
    // Use today's DOW to backdate the DOW of the 25th.
    const int8_t day = BCDToDec(TheDateTime.day);
    int8_t dow_25 = TheDateTime.wday - ( day - 25);
    if (dow_25 < 1)
       dow_25 += 7;
    
    int8_t dst_day = 32 - dow_25;
    
    dstActive = (TheDateTime.month == 0x10);
    
    if (day > dst_day || (day == dst_day && TheDateTime.hour > 2))
    {
      // Past DST adjustment time, toggle it.
      
      // This assumes that whenever the time is set on the DST adjustment day in the
      // fall, and it is after 2 AM that DST has been adjusted for already. 
      dstActive = !dstActive;
    }
  }
  return dstActive;
}

_Bool IsAlarmScheduled(struct AlarmSetting *alarm)
{
  if ((alarm->flags & ALARM_ACTIVE) == 0)
    return 0;
  
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
  
  if (TheDateTime.hour < alarm->hour || (TheDateTime.hour == alarm->hour && TheDateTime.min <= alarm->min))
  {
    // Alarm will trigger today
    nextAlarmDay = TheDateTime.wday - 1; // 0-6
  }
  else
  {
    // Alarm will trigger tomorrow.
    nextAlarmDay = TheDateTime.wday;
  }
    
  // Check if day matches
  return dayBits & (1 << nextAlarmDay);
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
  SI4702_PowerOn();
  
  SI4702_SetFrequency(TheGlobalSettings.radio.frequency);
  SI4702_SetVolume(TheGlobalSettings.radio.volume);
  
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
  }  
  
  SI4702_PowerOff();
  radioIsOn = 0;
  TheSleepTime = 0;
}

void AddOneBCD(uint8_t *value)
{
  (*value)++;
  
  if ((*value & 0x0f) == 0x0a)
    *value += 6;
}

void SubtractOneBCD(uint8_t *value)
{
  (*value)--;
  if ((*value & 0x0f) == 0x0f)
  {
    *value -= 6;
  }
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
      {
	v++;
	*editDigit = (*editDigit & 0xf0) | v;
      }
      else
      {
	*editDigit += 7;
      }
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
      if (*editDigit < editMaxValue)
	AddOneBCD(editDigit);
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
      {
	v--;
	*editDigit = (*editDigit & 0xf0) | v;
      }
      else
      {
	if (*editDigit > 0)
	  *editDigit-= 7; 
      }
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
	SubtractOneBCD(editDigit);
      break;
    }
  }
  
  if ((editMode & EDIT_MODE_ONEBASE) && (*editDigit == 0))
    *editDigit = 1;
} 

uint8_t alarm1Timeout = 0; // minutes
uint8_t alarm2Timeout = 0; // minutes
uint8_t napTimeout = 0; // minutes

_Bool alarm1Scheduled = 0, alarm2Scheduled = 0;

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
  uint16_t alarmMinOfDay = alarm->hour * 60 + alarm->min;
  
  uint16_t prevMinOfDay = ThePreviousDateTime.hour * 60 + ThePreviousDateTime.min;
  
  if (prevMinOfDay >= alarmMinOfDay)
    return 0; // already triggered
    
  uint16_t curMinOfDay = TheDateTime.hour * 60 + TheDateTime.min;
  
  if (curMinOfDay < prevMinOfDay)
    curMinOfDay += 60*24;
    
  if (curMinOfDay < alarmMinOfDay)
    return 0; // not yet triggered
  
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
    
    TheGlobalSettings.brightness = 3; // Default brightness.
    UpdateDOW(); // Don't assume the DOW is valid
    Write_DS1307_DateTime(); // This will reset the seconds divider, but that's acceptable as this is not expected to be called on a valid set clock anyway..
    TheGlobalSettings.dstActive = DetermineDST(); // Seed the DST state.    
    WriteGlobalSettings();
  }
  else
  {
    // Settings valid, see if DST rolled over while running on standby. 
    _Bool currentDST =  DetermineDST();
    if (currentDST != TheGlobalSettings.dstActive)
    {
      // Retro-actively apply DST
      
      if (currentDST)
      {
	// Go to DST (add one hour)
	AddOneBCD(&TheDateTime.hour);
	if (TheDateTime.hour == 0x24)
	{
	  TheDateTime.hour = 0;
	  AddOneBCD(&TheDateTime.day);
	  if (TheDateTime.day > GetDaysPerMonth())
	  {
	    TheDateTime.day = 1;
	    AddOneBCD(&TheDateTime.month);
	    if (TheDateTime.month == 0x13)
	    {
	      TheDateTime.month = 0;
	      AddOneBCD(&TheDateTime.year);
	    }
	  }
	}
      }
      else
      {
	// Go to normal time (subtract one hour)
	SubtractOneBCD(&TheDateTime.hour);
	if (TheDateTime.hour > 0x23) // Roll-over
	{
	  TheDateTime.hour = 0x23;
	  SubtractOneBCD(&TheDateTime.day);
	  if (TheDateTime.day == 0)
	  {
	    SubtractOneBCD(&TheDateTime.month);
	    if (TheDateTime.month == 0)
	      TheDateTime.month = 0x12;
	      
	    SubtractOneBCD(&TheDateTime.year);
	    TheDateTime.day = GetDaysPerMonth();
	  }
	}
      }
      
      TheGlobalSettings.dstActive = currentDST;
      Write_DS1307_DateTime(); // This will reset the seconds divider, but that's acceptable as this is not expected to be called very often..	    
      WriteGlobalSettings();
    }
  }

  SetBrightness(TheGlobalSettings.brightness);
  
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
  uint8_t modeTimeout = 0; // seconds
  uint8_t writeSettingTimeout = 0; // seconds
  
  _Bool timePollAllowed = 1;
  
  sei(); // Enable interrupts. This will immediately trigger a port change interrupt; sink these events.
  
  struct AlarmSetting alarmBeingModified;
  _Bool adjustAlarm1 = false;
  
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

    if (eventToHandle & CLOCK_UPDATE)
    {
      if (writeSettingTimeout)
      {
	--writeSettingTimeout;
	if (writeSettingTimeout == 0)
	{
	  WriteGlobalSettings();
	}
      }
      
      if (modeTimeout && --modeTimeout == 0)
      {
	 newDeviceMode = modeShowTime;
      }
    
      if (timePollAllowed)
      {
	Read_DS1307_DateTime();
	updateScreen = 1;
	
	// See if hours rolled over and it is a sunday
	if (ThePreviousDateTime.min > TheDateTime.min && TheDateTime.day >= 0x25 && TheDateTime.wday == 7)
	{
	  // See if DST must be adjusted: Spring
	  if (TheDateTime.hour == 2 && TheDateTime.month == 3 && !TheGlobalSettings.dstActive)
	  {
	    // Yup.
	    TheGlobalSettings.dstActive = 1;
	    TheDateTime.hour = 3;
	    Write_DS1307_HoursOnly();
	    writeSettingTimeout = 5;
	  }
	  // else see if DST must be adjusted: Fall
	  else if (TheDateTime.hour == 3 && TheDateTime.month == 0x10 && TheGlobalSettings.dstActive)
	  {
	    // Yup
	    TheGlobalSettings.dstActive = 0;
	    TheDateTime.hour = 2;
	    Write_DS1307_HoursOnly();
	    writeSettingTimeout = 5;
	  }
	}
	
	// See if alarms must timeout
	if (ThePreviousDateTime.sec > TheDateTime.sec)
	{
	  // Second rollover.
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
	  
	  if (TheSleepTime && deviceMode != modeAdjustSleep)
	  {
	    TheSleepTime--;
	    if (TheSleepTime == 0)
	    {
	      RadioOff();
	      if (deviceMode == modeShowRadio || deviceMode == modeShowRadio_Volume)
		newDeviceMode = modeShowTime;
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
	  
	  // See if nap timer must fire
	  if (TheNapTime && deviceMode != modeAdjustNap)
	  {
	    TheNapTime--;
	    if (TheNapTime == 0)
	    {
	      BeepOn();
	      napTimeout = ALARM_BEEP_TIMEOUT;
	      newDeviceMode = modeAlarmFiring;
	    }
	  }
	  
	  // See if alarms must fire.
	  if (alarm1Scheduled && AlarmTriggered(&TheGlobalSettings.alarm1))
	  {
	    if (TheGlobalSettings.alarm1.flags & ALARM_TYPE_RADIO)
	    {
	      RadioOn();
	      alarm1Timeout = ALARM_RADIO_TIMEOUT;
	    }
	    else
	    {
	      BeepOn();
	      alarm1Timeout = ALARM_BEEP_TIMEOUT;
	    }
	    
	    newDeviceMode = modeAlarmFiring;
	  }
	  
	  if ( alarm2Scheduled && AlarmTriggered(&TheGlobalSettings.alarm2))
	  {
	    if (TheGlobalSettings.alarm2.flags & ALARM_TYPE_RADIO)
	    {
	      RadioOn();
	      alarm2Timeout = ALARM_RADIO_TIMEOUT;
	    }
	    else
	    {
	      BeepOn();
	      alarm2Timeout = ALARM_BEEP_TIMEOUT;
	    }
	    
	    newDeviceMode = modeAlarmFiring;
	  }
        }	
	ThePreviousDateTime = TheDateTime;
	
	if (deviceMode < modeShowAlarm1)
	{
	  alarm1Scheduled = IsAlarmScheduled(&TheGlobalSettings.alarm1);
	  alarm2Scheduled = IsAlarmScheduled(&TheGlobalSettings.alarm2);
	  
	  Renderer_SetLed(TheNapTime > 0 ? LED_ON : LED_OFF, alarm1Scheduled ? LED_ON : LED_OFF,  alarm2Scheduled ? LED_ON : LED_OFF, TheSleepTime > 0 ? LED_ON : TheSleepTime > 0 ? LED_ON : LED_OFF);
	}
      }
    }
    
    // Handle keypresses
    switch ( deviceMode )
    {
      case modeShowTime:
      {
	if (eventToHandle & BUTTON5_CLICK)
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
	  break;
	}
	if (longPressEvent.longPress & BUTTON1_CLICK)
	{
	  // adjust time
	  newDeviceMode = modeAdjustYearTens;
	} else if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  newDeviceMode = modeShowDate;
	} else if (TheGlobalSettings.brightness != 0 && longPressEvent.repPress & BUTTON3_CLICK)
	{
	  TheGlobalSettings.brightness--;
	  SetBrightness(TheGlobalSettings.brightness);
	  writeSettingTimeout = 5;
	} else if (TheGlobalSettings.brightness < 15 && longPressEvent.repPress & BUTTON4_CLICK)
	{
	  TheGlobalSettings.brightness++;
	  SetBrightness(TheGlobalSettings.brightness);
	  writeSettingTimeout = 5;
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
	  newDeviceMode = (radioIsOn? modeShowRadio : modeShowAlarm1);
	}
	break;
      }
      case modeShowRadio:
	if (eventToHandle & BUTTON5_CLICK)
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
	
	if (eventToHandle & BUTTON1_CLICK)
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
	    if (eventToHandle & BUTTON3_CLICK)
	    {
	      writeSettingTimeout = 0; // Postpone writing settings, the SI4702 prefers the I2C bus t be quiet
	      SI4702_Tune(0);
	    } else if (eventToHandle & BUTTON4_CLICK)
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
	    if ((eventToHandle & BUTTON3_CLICK) || (longPressEvent.repPress & BUTTON3_CLICK))
	    {
	      if (TheGlobalSettings.radio.volume > 1)
	      {
		TheGlobalSettings.radio.volume--;
		SI4702_SetVolume(TheGlobalSettings.radio.volume);
		writeSettingTimeout = 5;
		Renderer_Update_Secondary();
	      }
	    } else if ((eventToHandle & BUTTON4_CLICK) || (longPressEvent.repPress & BUTTON4_CLICK))
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
	if ( eventToHandle & BUTTON2_CLICK )
	{
	  modeTimeout = SHOW_ALARM_TIMEOUT;
	  TheGlobalSettings.alarm1.flags ^= ALARM_ACTIVE;
	  Renderer_SetLed(TheNapTime > 0 ? LED_ON : LED_OFF, TheGlobalSettings.alarm1.flags & ALARM_ACTIVE ? LED_BLINK_LONG : LED_BLINK_SHORT, IsAlarmScheduled( &TheGlobalSettings.alarm2) ? LED_ON : LED_OFF, TheSleepTime > 0 ? LED_ON : LED_OFF);	
	}
	
	if ((eventToHandle & CLOCK_UPDATE) && (--modeTimeout == 0) )
	{
	  if (radioIsOn)
	    newDeviceMode = modeShowRadio;
	  else
	    newDeviceMode = modeShowTime;
	}
	
	if (longPressEvent.longPress & BUTTON1_CLICK)
	{
	  // adjust alarm
	  alarmBeingModified = TheGlobalSettings.alarm1;
	  adjustAlarm1 = 1;
	  newDeviceMode = modeAdjustHoursTens_Alarm;
	  
	} else if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  newDeviceMode = modeShowAlarm2;
	}
	break;
	
      case modeShowAlarm2:
	if ( eventToHandle & BUTTON2_CLICK )
	{
	  modeTimeout = SHOW_ALARM_TIMEOUT;
	  TheGlobalSettings.alarm2.flags ^= ALARM_ACTIVE;
	  Renderer_SetLed(TheNapTime > 0 ? LED_ON : LED_OFF, IsAlarmScheduled( &TheGlobalSettings.alarm1) ? LED_ON : LED_OFF, TheGlobalSettings.alarm2.flags & ALARM_ACTIVE ? LED_BLINK_LONG : LED_BLINK_SHORT, TheSleepTime > 0 ? LED_ON : LED_OFF);
	}
	
	if ((eventToHandle & CLOCK_UPDATE) && (--modeTimeout == 0) )
	{
	  if (radioIsOn)
	    newDeviceMode = modeShowRadio;
	  else
	    newDeviceMode = modeShowTime;
	}
	
	if (longPressEvent.longPress & BUTTON1_CLICK)
	{
	  // adjust alarm
	  alarmBeingModified = TheGlobalSettings.alarm2;
	  adjustAlarm1 = 0;
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
      case modeAlarmFiring:
	{
	  if (eventToHandle & (BUTTON1_CLICK | BUTTON2_CLICK | BUTTON3_CLICK | BUTTON4_CLICK | BUTTON5_CLICK))
	  {
	    if (napTimeout)
	    {
	      napTimeout = 0; 
	      BeepOff();
	    }
	    else
	    {
	      // Consume one alarm.
	      _Bool silenceAlarm1 = (alarm1Timeout);
	      
	      if (alarm1Timeout && alarm2Timeout)
	      {
		// Both alarms are active: Kill the annoying one.
		silenceAlarm1 = (TheGlobalSettings.alarm2.flags & ALARM_TYPE_RADIO); // Alarm 2 is a radio, so kill alarm1. Otherwise, kill alarm2.
	      }
	      
	      if (silenceAlarm1)
	      {
		alarm1Timeout = 0;
		SilenceAlarm(&TheGlobalSettings.alarm1);
	      }
	      else
	      {
		alarm2Timeout = 0;
		SilenceAlarm(&TheGlobalSettings.alarm2);
	      }
	    }
	  }
	  if (!alarm1Timeout && !alarm2Timeout && !napTimeout) 
	  {
	    MarkLongPressHandled(eventToHandle); // don't generate shortpresses. 
	    newDeviceMode = modeShowTime;
	  }
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
	    TheGlobalSettings.dstActive = DetermineDST();
	    writeSettingTimeout = 5;
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
      case modeAdjustHoursTens_Alarm:
      case modeAdjustHoursOnes_Alarm:
      case modeAdjustMinsTens_Alarm:
      case modeAdjustMinsOnes_Alarm:
      {
	if (longPressEvent.longPress & BUTTON1_CLICK)
	{
	  // Abort adjusting alarm
	  newDeviceMode = adjustAlarm1 ? modeShowAlarm1 : modeShowAlarm2;
	} else if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  newDeviceMode++;
	}
	else if (eventToHandle & BUTTON3_CLICK)
	{
	  updateScreen = 1;
	  HandleEditDown(editMode, editDigit, editMaxValue);
	}
	else if (eventToHandle & BUTTON4_CLICK)
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
	  newDeviceMode = adjustAlarm1 ? modeShowAlarm1 : modeShowAlarm2;
	} else if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  newDeviceMode++;
	}
	else if (eventToHandle & BUTTON3_CLICK)
	{
	  updateScreen = 1;
	  uint8_t newDays = (alarmBeingModified.flags - 4 ) & ALARM_DAY_BITS;
	  alarmBeingModified.flags &= ~ALARM_DAY_BITS;
	  alarmBeingModified.flags |= newDays;
	}
	else if (eventToHandle & BUTTON4_CLICK)
	{
	  updateScreen = 1;
	  uint8_t newDays = (alarmBeingModified.flags + 4 ) & ALARM_DAY_BITS;
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
	  newDeviceMode = adjustAlarm1 ? modeShowAlarm1 : modeShowAlarm2;
	} else if (longPressEvent.shortPress & BUTTON1_CLICK)
	{
	  if (adjustAlarm1)
	  {
	    TheGlobalSettings.alarm1 = alarmBeingModified;
	    newDeviceMode = modeShowAlarm1;
	  }
	  else
	  {
	    TheGlobalSettings.alarm2 = alarmBeingModified;
	    newDeviceMode = modeShowAlarm2;
	  }
	    
	  MarkLongPressHandled(BUTTON1_CLICK);
	  writeSettingTimeout = 5;
	}
	else if (eventToHandle & (BUTTON3_CLICK | BUTTON4_CLICK))
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
	} else if (eventToHandle & BUTTON5_CLICK)
	{
	  modeTimeout = SHOW_ALARM_TIMEOUT; 
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
	} else if (eventToHandle & BUTTON5_CLICK)
	{
	  modeTimeout = SHOW_ALARM_TIMEOUT; 
	  HandleCycleTime(&TheNapTime);
	  updateScreen = 1;
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
	  modeTimeout = 255;
	  mainMode = MAIN_MODE_DATE;
          secMode = SECONDARY_MODE_YEAR;
	  timePollAllowed = 0;
	  
          Renderer_SetFlashMask(0x2); 
          editDigit = &TheDateTime.year;
          editMode = EDIT_MODE_TENS | EDIT_MODE_ONEBASE;
          editMaxValue = 0x99;
          break;
        case modeAdjustYearOnes:
	  modeTimeout = 255;
          Renderer_SetFlashMask(0x1); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustMonth:
	  modeTimeout = 255;
          editMode = EDIT_MODE_NUMBER | EDIT_MODE_ONEBASE; 
          editDigit = &TheDateTime.month;
          editMaxValue = 0x12;
          Renderer_SetFlashMask(0x30); 
          break;
        case modeAdjustDayTens:
	  modeTimeout = 255;
          editDigit = &TheDateTime.day;
          editMaxValue = GetDaysPerMonth();
          editMode = EDIT_MODE_TENS | EDIT_MODE_ONEBASE;
          Renderer_SetFlashMask(0x80); 
          break;
        case modeAdjustDayOnes:
	  modeTimeout = 255;
          editMode = EDIT_MODE_ONES | EDIT_MODE_ONEBASE;
          Renderer_SetFlashMask(0x40); 
          break;
        case modeAdjustHoursTens:
	  modeTimeout = 255;
	  mainMode = MAIN_MODE_TIME;
          secMode = SECONDARY_MODE_SEC;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x23;
          editDigit = &TheDateTime.hour;
          Renderer_SetFlashMask(0x80); 
          break;
        case modeAdjustHoursOnes:
	  modeTimeout = 255;
          Renderer_SetFlashMask(0x40); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeAdjustMinsTens:
	  modeTimeout = 255;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x59;
          editDigit = &TheDateTime.min;
          Renderer_SetFlashMask(0x20); 
          break;
        case modeAdjustMinsOnes:
	  modeTimeout = 255;
          Renderer_SetFlashMask(0x10); 
          editMode = EDIT_MODE_ONES;
          break;
        case modeShowTime:
	  modeTimeout = 0;
	  Renderer_SetFlashMask(0);
	  timePollAllowed = 1;
	  editMode = 0;	  
	  secMode = SECONDARY_MODE_SEC;
	  mainMode = MAIN_MODE_TIME;
          break;
	case modeShowDate:
	  modeTimeout = 3;
	  Renderer_SetFlashMask(0);
	  editMode = 0;
	  mainMode = MAIN_MODE_DATE;
	  secMode = SECONDARY_MODE_YEAR;
	  break;
	case modeShowRadio:
	  timePollAllowed = 1;
	  modeTimeout = 0;
	  mainMode = MAIN_MODE_TIME;
	  secMode = SECONDARY_MODE_RADIO;
	  Renderer_Update_Secondary();
	  break;
	case modeShowRadio_Volume:
	  timePollAllowed = 1;
	  modeTimeout = 0;
	  mainMode = MAIN_MODE_TIME;
	  secMode = SECONDARY_MODE_VOLUME;
	  Renderer_Update_Secondary();
	  break;
	case modeAlarmFiring:
	  modeTimeout = 0;
	  Renderer_SetFlashMask(0xff);
	  timePollAllowed = 1;
	  editMode = 0;	  
	  secMode = SECONDARY_MODE_SEC;
	  mainMode = MAIN_MODE_TIME;
          break;
	case modeShowAlarm1:
	{
	  Renderer_SetFlashMask(0x00); 
	  timePollAllowed = 1;
	  modeTimeout = SHOW_ALARM_TIMEOUT;
	  mainMode = MAIN_MODE_ALARM;
	  secMode = SECONDARY_MODE_ALARM;
	  Renderer_SetAlarmStruct(&TheGlobalSettings.alarm1);
	  Renderer_SetLed(TheNapTime > 0 ? LED_ON : LED_OFF, TheGlobalSettings.alarm1.flags & ALARM_ACTIVE ? LED_BLINK_LONG : LED_BLINK_SHORT, IsAlarmScheduled( &TheGlobalSettings.alarm2) ? LED_ON : LED_OFF, TheSleepTime > 0 ? LED_ON : LED_OFF);
	  Renderer_Update_Secondary();
	  break;
	}
	case modeShowAlarm2:
	  Renderer_SetFlashMask(0x00); 
	  timePollAllowed = 1;
	  modeTimeout = SHOW_ALARM_TIMEOUT;
	  mainMode = MAIN_MODE_ALARM;
	  secMode = SECONDARY_MODE_ALARM;
	  Renderer_SetAlarmStruct(&TheGlobalSettings.alarm2);
	  Renderer_SetLed(TheNapTime > 0 ? LED_ON : LED_OFF, IsAlarmScheduled( &TheGlobalSettings.alarm1) ? LED_ON : LED_OFF, TheGlobalSettings.alarm2.flags & ALARM_ACTIVE ? LED_BLINK_LONG : LED_BLINK_SHORT, TheSleepTime > 0 ? LED_ON : LED_OFF);
	  
	  Renderer_Update_Secondary();
	  break;
	case modeAdjustHoursTens_Alarm:
	  modeTimeout = 255;
	  Renderer_SetFlashMask(0x80); 
	  Renderer_SetAlarmStruct(&alarmBeingModified);
	  editDigit = &alarmBeingModified.hour;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x23;
	  break;
	case modeAdjustHoursOnes_Alarm:
	  modeTimeout = 255;
	  Renderer_SetFlashMask(0x40); 
          editMode = EDIT_MODE_ONES;
	  break;
	case modeAdjustMinsTens_Alarm:
	  modeTimeout = 255;
	  Renderer_SetFlashMask(0x20); 
	  editDigit = &alarmBeingModified.min;
          editMode = EDIT_MODE_TENS;
          editMaxValue = 0x59;
	  break;
	case modeAdjustMinsOnes_Alarm:
	  modeTimeout = 255;
	  Renderer_SetFlashMask(0x10); 
          editMode = EDIT_MODE_ONES;
	  break;
	case modeAdjustDays_Alarm:
	  modeTimeout = 255;
	  Renderer_SetFlashMask(0x100); 
          editMode = 0;
	  break;
	case modeAdjustType_Alarm:
	  modeTimeout = 255;
	  Renderer_SetFlashMask(0x0f);
	  break;
	case modeAdjustNap: 
	  if (TheNapTime == 0)
	    TheNapTime = INITIAL_NAPTIME;
	  modeTimeout = SHOW_ALARM_TIMEOUT; 
	  mainMode = MAIN_MODE_NAP;
	  secMode = SECONDARY_MODE_NAP;
	  Renderer_SetLed(LED_BLINK_SHORT, alarm1Scheduled ? LED_ON : LED_OFF,  alarm2Scheduled ? LED_ON : LED_OFF, TheSleepTime > 0 ? LED_ON : TheSleepTime > 0 ? LED_ON : LED_OFF);
	  break;
	case modeAdjustSleep:
	  if (TheSleepTime == 0)
	    TheSleepTime = INITIAL_SLEEPTIME;
	  modeTimeout = SHOW_ALARM_TIMEOUT; 
	  mainMode = MAIN_MODE_SLEEP;
	  secMode = SECONDARY_MODE_SLEEP;
	  Renderer_SetLed(TheNapTime > 0 ? LED_ON : LED_OFF, alarm1Scheduled ? LED_ON : LED_OFF,  alarm2Scheduled ? LED_ON : LED_OFF, LED_BLINK_SHORT);
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

    if (eventToHandle & CLOCK_TICK)
    {
      if (radioIsOn && Poll_SI4702() && deviceMode == modeShowRadio)
      {
	// Radio is done seeking or tuning
	Renderer_Update_Secondary();
	TheGlobalSettings.radio.frequency = SI4702_GetFrequency();
	writeSettingTimeout = 5;
      }
      
      Renderer_Tick(secMode);
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
