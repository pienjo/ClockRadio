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
#include <stdio.h>
#include <avr/sleep.h>

#include <avr/avr_mcu_section.h>
#include "../Timefuncs.h"
#include "../DateTime.h"
#include "../BCDFuncs.h"
#include "../settings.h"
#include <avr/pgmspace.h>

AVR_MCU(F_CPU, "atmega168p");

static int uart_putchar(char c, FILE *stream) 
{
  
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
  return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);

_Bool errorOccurred = 0;

const uint8_t PROGMEM DOW_tests[] = 
{
  // day, month, year, expected
  0x1, 0x12, 0x00, 5,
  0x1, 0x05, 0x17, 1,
  0x1, 0x01, 0x18, 1,
  0x2, 0x01, 0x18, 2,
  0x3, 0x01, 0x18, 3,
  0x4, 0x01, 0x18, 4,
  0x5, 0x01, 0x18, 5,
  0x6, 0x01, 0x18, 6,
  0x7, 0x01, 0x18, 7,
  0x8, 0x01, 0x18, 1,
  0x31,0x01, 0x18, 3,
  0x1, 0x02, 0x18, 4,
  0x6, 0x04, 0x18, 5, 
  0, 0, 0,  0,
};

static void Test_GetDayOfWeek()
{
  static const char PROGMEM title[] = "GetDayofWeek...\n";
  printf_P(title);
  for( int testIdx = 0; ; ++testIdx)
  {
    const uint8_t day   = pgm_read_byte( 4 * testIdx + DOW_tests),
                  month = pgm_read_byte( 4 * testIdx + DOW_tests + 1),
                  year  = pgm_read_byte( 4 * testIdx + DOW_tests + 2),
                  expect= pgm_read_byte( 4 * testIdx + DOW_tests + 3);

    if (day == 0)
      break;

    const uint8_t actual = GetDayOfWeek(day, month, year);

    if (actual != expect)
    {
      static const char PROGMEM fmt[] ="%02x/%02x/20%02x: Expected %2x, got %2x\n";
      printf_P(fmt, day, month, year, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      static const char PROGMEM fmt[] = "%02x/%02x/20%02x: OK (%d)\n";
      printf_P(fmt, day, month, year, expect);
    }

  }
}

const uint8_t PROGMEM DPM_tests[] =  {
  /* Month, year (BCD), expcted */
  0x1, 1, 0x31,
  0x2, 1, 0x28,
  0x3, 1, 0x31,
  0x4, 1, 0x30,
  0x5, 1, 0x31,
  0x6, 1, 0x30,
  0x7, 1, 0x31,
  0x8, 1, 0x31,
  0x9, 1, 0x30,
  0x10, 1, 0x31,
  0x11, 1, 0x30,
  0x12, 1, 0x31,
  0, 0, 0
};

static void Test_GetDaysPerMonth()
{
  static const char PROGMEM title []= "GetDayPerMonth\n";
  printf_P(title);
  
  for( int testIdx = 0; ; ++testIdx)
  {
    const uint8_t month = pgm_read_byte( 3 * testIdx + DPM_tests + 0),
                  year  = pgm_read_byte( 3 * testIdx + DPM_tests + 1),
                  expect= pgm_read_byte( 3 * testIdx + DPM_tests + 2);

    if (month == 0)
      break;

    const uint8_t actual = GetDaysPerMonth(month, year);
    
    if (actual != expect)
    {
      static const char PROGMEM fmt[]="%02x/20%02x: Expected %02x, got %02x\n";
      printf_P(fmt,month, year, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      static const char PROGMEM fmt[]="%02x/20%02x: OK (%02x)\n";
      printf_P(fmt,month, year, expect);
    }

  }
}

const uint8_t PROGMEM DOLS_tests[] =  {
  /* Month, year (BCD), expcted */
  0x1, 1, 0x28,
  0x2, 1, 0x25,
  0x3, 1, 0x25,
  0x4, 1, 0x29,
  0x5, 1, 0x27,
  0x6, 1, 0x24,
  0x7, 1, 0x29,
  0x8, 1, 0x26,
  0x9, 1, 0x30,
  0x10, 1, 0x28,
  0x11, 1, 0x25,
  0x12, 1, 0x30,
  0x02, 0x20, 0x23,
  0, 0, 0
};

static void Test_GetDateOfLastSunday()
{
  static const char PROGMEM title []= "GetDateOfLastSunday..\n";
  printf_P(title);
  
  for( int testIdx = 0; ; ++testIdx)
  {
    const uint8_t month = pgm_read_byte( 3 * testIdx + DOLS_tests + 0),
                  year  = pgm_read_byte( 3 * testIdx + DOLS_tests + 1),
                  expect= pgm_read_byte( 3 * testIdx + DOLS_tests + 2);

    if (month == 0)
      break;

    const uint8_t actual = GetDateOfLastSunday(month, year);
    
    if (actual != expect)
    {
      static const char PROGMEM fmt[]="%02x/20%02x: Expected %02x, got %02x\n";
      printf_P(fmt,month, year, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      static const char PROGMEM fmt[]="%02x/20%02x: OK (%02x)\n";
      printf_P(fmt,month, year, expect);
    }

  }
}

const uint8_t PROGMEM BCDToBin_tests[] =  
{
  0x1,   1,
  0x9,   9,
  0x10, 10,
  0x19, 19,
  0x20, 20,
  0x99, 99,
  0,0 
};

static void Test_BCDToBin()
{
  static const char PROGMEM title []= "BCDToBin..\n";
  printf_P(title);
    
  for (int testIdx = 0;; ++testIdx)
  {
    const uint8_t input = pgm_read_byte(2 * testIdx + BCDToBin_tests + 0),
                  expect = pgm_read_byte(2 * testIdx + BCDToBin_tests + 1);
                  
    if (input == 0)
      break;
      
    const uint8_t actual = BCDToBin(input);
    
    if (actual != expect)
    {
      static const char PROGMEM fmt[]="0x%02x: Expected %d, got %d\n";
      printf_P(fmt, input, expect, actual);
      errorOccurred = 1;
    }
    else 
    { 
      static const char PROGMEM fmt[]="0x%02x: OK (%d)\n";
      printf_P(fmt, input, actual);
    }
  }
}

const uint8_t PROGMEM BCDAdd_tests[] =  {
    // left, right, expected
    0x00, 0x12, 0x12,
    0x13, 0x00, 0x13,
    0x04, 0x06, 0x10,
    0x05, 0x06, 0x11,
    0x09, 0x09, 0x18,
    0x19, 0x05, 0x24,
    0x54, 0x55, 0x09, // overflow
    0x32, 0x99, 0x31, // overflow  
    0xff, 0xff, 0xff
};

static void Test_BCDAdd() 
{
  static const char PROGMEM title []= "BCDAdd..\n" ;
  printf_P(title);
  
  for( int testIdx = 0; ; ++testIdx)
  {
    const uint8_t left = pgm_read_byte( 3 * testIdx + BCDAdd_tests + 0),
                  right = pgm_read_byte( 3 * testIdx + BCDAdd_tests + 1),
                  expect= pgm_read_byte( 3 * testIdx + BCDAdd_tests + 2);

    if (left == 0xff)
      break;
    
    const uint8_t actual = BCDAdd(left, right);
    if (actual != expect)
    {
      static const char PROGMEM fmt[]="0x%02x + 0x%02x: Expected 0x%02x, got 0x%02x\n";
      printf_P(fmt, left, right, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      static const char PROGMEM fmt[]="0x%02x + 0x%02x: OK(0x%02x)\n";
      printf_P(fmt, left, right, actual);
    }
  }
}


const uint8_t PROGMEM BCDSub_tests[] =  {
    // left, right, expected
    0x09, 0x04, 0x05,
    0x90, 0x50, 0x40,
    0x10, 0x01, 0x09,
    0x00, 0x99, 0x01,
    0xff, 0xff, 0xff
};

static void Test_BCDSub() 
{
  static const char PROGMEM title []= "BCDSub..\n" ;
  printf_P(title);
  
  for( int testIdx = 0; ; ++testIdx)
  {
    const uint8_t left = pgm_read_byte( 3 * testIdx + BCDSub_tests + 0),
                  right = pgm_read_byte( 3 * testIdx + BCDSub_tests + 1),
                  expect= pgm_read_byte( 3 * testIdx + BCDSub_tests + 2);

    if (left == 0xff)
      break;
    
    const uint8_t actual = BCDSub(left, right);
    if (actual != expect)
    {
      static const char PROGMEM fmt[]="0x%02x - 0x%02x: Expected 0x%02x, got 0x%02x\n";
      printf_P(fmt, left, right, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      static const char PROGMEM fmt[]="0x%02x - 0x%02x: OK(0x%02x)\n";
      printf_P(fmt, left, right, actual);
    }
  }
}

const uint8_t PROGMEM HandleEditUpTests[] = 
{
  /* input, editmode, maxvalue, expected */
  0x00, EDIT_MODE_ONES, 0x99, 0x01,
  0x00, EDIT_MODE_TENS, 0x99, 0x10,
  0x59, EDIT_MODE_ONES, 0x59, 0x00,
  0x59, EDIT_MODE_TENS, 0x59, 0x09,
  0x57, EDIT_MODE_TENS, 0x59, 0x07,
  0x12, EDIT_MODE_ONES | EDIT_MODE_ONEBASE, 0x12, 0x12,
  0x12, EDIT_MODE_TENS | EDIT_MODE_ONEBASE, 0x12, 0x12,
  0x59, EDIT_MODE_TENS, 0x59, 0x09,
  0x99, EDIT_MODE_ONES, 0x99, 0x00,
  0x99, EDIT_MODE_TENS, 0x99, 0x09,
  0x97, EDIT_MODE_TENS, 0x99, 0x07,
  0xff, 0xff, 0xff, 0xff
};

static void Test_HandleEditUp()
{
  static const char PROGMEM title []= "HandleEditUp...\n" ;
  printf_P(title);
  for( int testIdx = 0; ; ++testIdx)
  {
    const uint8_t input = pgm_read_byte( 4 * testIdx + HandleEditUpTests + 0),
                  mode = pgm_read_byte( 4 * testIdx + HandleEditUpTests + 1),
                  max = pgm_read_byte( 4 * testIdx + HandleEditUpTests + 2),
                              expect = pgm_read_byte( 4 * testIdx + HandleEditUpTests + 3);

    if (input == 0xff)
      break;
    
    uint8_t actual = input;
    HandleEditUp(mode, &actual, max);

    if (actual != expect)
    {
      static const char PROGMEM fmt[]="0x%02x/0x%02x/0x%02x: Expected 0x%02x,got 0x%02x\n";
      printf_P(fmt, input, mode, max, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      static const char PROGMEM fmt[]="0x%02x / 0x%02x / 0x%02x: OK(0x%02x)\n";
      printf_P(fmt, input, mode, max, actual);
    }
    
  }
}
const uint8_t PROGMEM HandleEditDownTests[] = 
{
  /* input, editmode, maxvalue, expected */
  0x01, EDIT_MODE_ONES, 0x99, 0x00,
  0x10, EDIT_MODE_TENS, 0x99, 0x00,
  0x00, EDIT_MODE_ONES, 0x99, 0x99,
  0x00, EDIT_MODE_TENS, 0x99, 0x90,
  0x00, EDIT_MODE_ONES, 0x59, 0x59,
  0x00, EDIT_MODE_TENS, 0x59, 0x50,
  0x07, EDIT_MODE_TENS, 0x59, 0x57,
  0x1, EDIT_MODE_ONES | EDIT_MODE_ONEBASE, 0x12, 0x1,
  0x06, EDIT_MODE_TENS | EDIT_MODE_ONEBASE, 0x12, 0x1,
  0xff, 0xff, 0xff, 0xff
};

static void Test_HandleEditDown()
{
  static const char PROGMEM title []= "HandleEditDown...\n";
  printf_P(title);
  
  for( int testIdx = 0; ; ++testIdx)
  {
    const uint8_t input = pgm_read_byte( 4 * testIdx + HandleEditDownTests + 0),
                  mode = pgm_read_byte( 4 * testIdx + HandleEditDownTests + 1),
                  max = pgm_read_byte( 4 * testIdx + HandleEditDownTests + 2),
                              expect = pgm_read_byte( 4 * testIdx + HandleEditDownTests + 3);

    if (input == 0xff)
      break;
    
    uint8_t actual = input;
    HandleEditDown(mode, &actual, max);

    if (actual != expect)
    {
      static const char PROGMEM fmt[]="0x%02x/0x%02x/0x%02x: Expected 0x%02x,got 0x%02x\n";
      printf_P(fmt, input, mode, max, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      static const char PROGMEM fmt[]="0x%02x / 0x%02x / 0x%02x: OK(0x%02x)\n";
printf_P(fmt, input, mode, max, actual);
    }
    
  }
}
const uint8_t PROGMEM NormalizeHours_tests[] =
{
  // Year, Month, Day, Hour -> Year, Month, Day, Hour
  0x12, 0x10, 0x20, 0x21  , 0x12, 0x10, 0x20, 0x21, // No change 
  0x12, 0x10, 0x20, 0x25  , 0x12, 0x10, 0x21, 0x01, // Day rollover  (positive)
  0x12, 0x10, 0x31, 0x24  , 0x12, 0x11, 0x01, 0x00, // Month rollover (positive)
  0x12, 0x12, 0x31, 0x26  , 0x13, 0x01, 0x01, 0x02, // year rollover (positive)
  0x12, 0x10, 0x20, 0x99  , 0x12, 0x10, 0x19, 0x23, // Day rollover  (negative)
  0x12, 0x10, 0x01, 0x98  , 0x12, 0x09, 0x30, 0x22, // Month rollover (negative)
  0x12, 0x01, 0x01, 0x97  , 0x11, 0x12, 0x31, 0x21, // year rollover (negative)
  0xff
};

inline static _Bool timesAreEqual(struct DateTime *time1, struct DateTime *time2)
{
  if (time1->year != time2->year || time1->month != time2->month || time1->day != time2->day ||
      time1->hour != time2->hour || time1->min   != time2->min   || time1->sec != time2->sec)
      return 0;
  return 1;
}

static inline void printTime(struct DateTime *time)
{
  static const char PROGMEM fmt[]="%02x/%02x/%02x %02x:%02x:%02x";
  printf_P(fmt,time->year, time->month, time->day, time->hour, time->min, time->sec); 
}

void Test_NormalizeHours() 
{
  struct DateTime testTime;
  struct DateTime expectedTime;
  
  // constant among all tests
  testTime.min = 0x12;
  testTime.sec = 0x45;
  expectedTime.min = 0x12;
  expectedTime.sec = 0x45;
  
  static const char PROGMEM title []= "NormalizeHours..\n" ;
  printf_P(title);
  
  for( int testIdx = 0; ; ++testIdx)
  {
    testTime.year = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 0);
    if (testTime.year == 0xff)
      break;
    testTime.month = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 1);
    testTime.day = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 2);
    testTime.hour = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 3);
    
    expectedTime.year = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 4);
    expectedTime.month = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 5);
    expectedTime.day = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 6);
    expectedTime.hour = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 7);
    
    printTime(&testTime);
    
    NormalizeHours(&testTime);
    if (timesAreEqual(&testTime, &expectedTime))
    {
       static const char PROGMEM fmt1[]=" OK: (";
       printf_P(fmt1);
       printTime(&testTime);
       static const char PROGMEM fmt2[]=")\n";
       printf_P(fmt2);
    }
    else
    {
      static const char PROGMEM fmt[]=": Expected ";
      printf_P(fmt);
      printTime(&expectedTime);
      static const char PROGMEM fmt2[]=" Got ";
      printf_P(fmt2);
      printTime(&testTime);
      uart_putchar('\n', stdout);
      errorOccurred =1;
    }
    
  }

  // DOW tests
  for (uint8_t dow = 1; dow < 8; ++ dow)
  {
    // forward
    testTime.hour = 0x25;
    testTime.wday = dow;
    
    NormalizeHours(&testTime);
    uint8_t expect = (dow % 7) + 1;
    if (testTime.wday != expect)
    {
      static const char PROGMEM fmt[]="DOW error: %" PRIu8" positive, expect %d got %d\n";
      printf_P(fmt, dow, expect, testTime.wday);
      errorOccurred =1;
    }
    else
    {
      // backward
      testTime.hour = 0x99;
      testTime.wday = dow;
      
      NormalizeHours(&testTime);
      expect = ((dow + 5) %7 ) + 1;
      if (testTime.wday != expect)
      {
        static const char PROGMEM fmt[]="DOW error: %"PRIu8" negative, expect %d got %d\n";
        printf_P(fmt , dow, expect, testTime.wday);
        errorOccurred =1;
      }
    }
  }
}

const uint8_t PROGMEM IDA_tests[] =
{
  // Year, Month, Day, Hour, UTCTime -> DSTActive

  0x18, 0x4, 0x7, 0x12, 0, 1, // April is always DST.
  0x18, 0x4, 0x7, 0x12, 1, 1, // April is always DST.
  0x18, 0x1, 0x7, 0x12, 0, 0, // January is always not DST
  0x18, 0x1, 0x7, 0x12, 1, 0, // January is always not DST
  0x18, 0x11, 0x7, 0x11, 0, 0, // November is always not DST
  0x18, 0x11, 0x7, 0x11, 1, 0, // November is always not DST
  0x18, 0x3, 0x20, 0x12, 0, 0, // March, before transition date
  0x18, 0x3, 0x20, 0x12, 1, 0, // March, before transition date
  0x18, 0x3, 0x30, 0x12, 0, 1, // March, after transition date
  0x18, 0x3, 0x30, 0x12, 1, 1, // March, after transition date
  0x18, 0x3, 0x25, 0x00, 0, 0, // March on transation date but before time
  0x18, 0x3, 0x25, 0x00, 1, 0, // March on transation date but before time
  0x18, 0x3, 0x25, 0x03, 0, 1, // March on transation date but after time 
  0x18, 0x3, 0x25, 0x03, 1, 1, // March on transation date but after time 
  0x18, 0x10, 0x20, 0x12, 0, 1, // October, before transition date
  0x18, 0x10, 0x20, 0x12, 1, 1, // October, before transition date
  0x18, 0x10, 0x30, 0x12, 0, 0, // October, after transition date
  0x18, 0x10, 0x30, 0x12, 1, 0, // October, after transition date
  0x18, 0x10, 0x28, 0x00, 0, 1, // October on transation date but before time
  0x18, 0x10, 0x28, 0x00, 1, 1, // October on transation date but before time
  0x18, 0x10, 0x28, 0x02, 0, 0, // October on transation date but after time 
  0x18, 0x10, 0x28, 0x01, 1, 0, // October on transation date but after time 
  
  0xff
};

static void Test_IsDSTActive()
{
  static const char PROGMEM title []= "IsDSTActive..\n";
  printf_P(title);
  
  struct DateTime testTime;
  
  // constant among all tests
  testTime.min = 0x12;
  testTime.sec = 0x45;
  
  for( int testIdx = 0; ; ++testIdx)
  {
    testTime.year = pgm_read_byte( 6 * testIdx + IDA_tests + 0);
    if (testTime.year == 0xff)
      break;
   
    testTime.month = pgm_read_byte( 6 * testIdx + IDA_tests + 1);
    testTime.day = pgm_read_byte( 6 * testIdx + IDA_tests + 2);
    testTime.hour = pgm_read_byte( 6 * testIdx + IDA_tests + 3);
    _Bool utcActive = pgm_read_byte( 6 * testIdx + IDA_tests + 4);
    _Bool expected = pgm_read_byte( 6 * testIdx + IDA_tests + 5);
    _Bool actual = IsDSTActive(&testTime, utcActive);
    
    printTime(&testTime);
    uart_putchar('(', stdout);
    uart_putchar(utcActive ? 'U':'C', stdout);  
    uart_putchar(')', stdout);
    uart_putchar(':', stdout);
    uart_putchar(' ', stdout);
    
    if (actual == expected)
    {
      static const char PROGMEM fmt[]="OK (%d)\n";
      printf_P(fmt, expected);
    }
    else
    {
      static const char PROGMEM fmt[]="Expected %d, got %d\n";
      printf_P(fmt, expected, actual);
      errorOccurred = 1;
    }
  }
}

const uint8_t PROGMEM IIDO_tests[] = 
{
  // Month, hour, expected
  0x01, 0x06, 1,
  0x01, 0x12, 0,
  0x01, 0x18, 1,
  0x07, 0x04, 1,
  0x07, 0x06, 0,
  0x07, 0x12, 0,
  0x07, 0x18, 0,
  0x07, 0x22, 1,
  0xff
};

static void Test_IsItDarkOutside()
{
  static const char PROGMEM title []= "IsItDarkOutside";
  printf_P(title);

  struct DateTime testTime;
  
  // constant among all tests
  testTime.day = 0x05;
  testTime.year= 0x18;
  testTime.sec = 0x00;
  testTime.min = 0x10;
  
  for( int testIdx = 0; ; ++testIdx)
  {
    testTime.month = pgm_read_byte( 3 * testIdx + IIDO_tests + 0);
    if (testTime.month == 0xff)
      break;
    testTime.hour = pgm_read_byte( 3 * testIdx + IIDO_tests + 1);
    
    const _Bool expect = pgm_read_byte( 3 * testIdx + IIDO_tests + 2);
    
    const _Bool actual = ItIsDarkOutside(&testTime);
    
    printTime(&testTime);
    if (expect == actual)
    {
      static const char PROGMEM fmt[]=": OK (%d)\n";
      printf_P(fmt, actual);
    }
    else
    {
      static const char PROGMEM fmt[]=": Expected %d, got %d\n";
      printf_P(fmt, expect, actual);
      errorOccurred = 1;
    }
  }
  
}

struct GlobalSettings TheGlobalSettings;

void Test_GetActiveBrightness()
{
  static const char PROGMEM title []= "Test_GetActiveBrightness..\n";
  printf_P(title);
  
  TheGlobalSettings.brightness = 10;
  TheGlobalSettings.brightness_night = 3;
  
  struct DateTime testTime;
  
  // Midnight
  testTime.year = 0x00;
  testTime.month = 0x07;
  testTime.day = 0x01;
  testTime.hour = 0x00;
  testTime.min = 0x00;
  testTime.sec = 0x00;
  
  printTime(&testTime);
  uint8_t actual = GetActiveBrightness(&testTime);
  if(actual != TheGlobalSettings.brightness_night)
  {
    static const char PROGMEM fmt[]=": Expected %d, got %d\n";
    printf_P(fmt, TheGlobalSettings.brightness_night, actual);
    errorOccurred = 1;
  }
  else
  {
    static const char PROGMEM fmt[]=": OK\n";
    printf_P(fmt);
  }
  
  // Midday
  testTime.hour = 0x12;
  
  printTime(&testTime);
  actual = GetActiveBrightness(&testTime);
  if(actual != TheGlobalSettings.brightness)
  {
    static const char PROGMEM fmt[]=": Expected %d, got %d\n";
    printf_P(fmt, TheGlobalSettings.brightness, actual);
    errorOccurred = 1;
  }
  else
  {
    static const char PROGMEM fmt[]=": OK\n";
    printf_P(fmt);
  }
}

void Test_IncreaseBrightness()
{
  static const char PROGMEM title []= "IncreaseBrightness..\n";
  printf_P(title);
      
  const uint8_t initialBrightness = 10, initialNightBrightness = 3;
  
  TheGlobalSettings.brightness = initialBrightness;
  TheGlobalSettings.brightness_night = initialNightBrightness;

  struct DateTime testTime;
  
  // Midnight
  testTime.year = 0x00;
  testTime.month = 0x07;
  testTime.day = 0x01;
  testTime.hour = 0x00;
  testTime.min = 0x00;
  testTime.sec = 0x00;
  
  printTime(&testTime);
  
  IncreaseBrightness(&testTime);
  
  if (TheGlobalSettings.brightness != initialBrightness)
  {
    static const char PROGMEM fmt[]=": Wrong setting changed.\n";
    printf_P(fmt);
    errorOccurred = 1;
  }
  else
  {
    if (TheGlobalSettings.brightness_night != initialNightBrightness + 1)
    {
      static const char PROGMEM fmt[]=": Expected %d, got %d\n";
      printf_P(fmt, initialNightBrightness + 1, TheGlobalSettings.brightness_night);
      errorOccurred = 1;
    }
    else
    {
      for (int i = 0; i < 15; ++i)
        IncreaseBrightness(&testTime);
        
      if (TheGlobalSettings.brightness_night != 15)
      {
        static const char PROGMEM fmt[]=": Setting didn't clip, got %d\n";
        printf_P(fmt, TheGlobalSettings.brightness_night);
        errorOccurred = 1;
      }
      else
      {
        static const char PROGMEM fmt[]=": OK\n";
        printf_P(fmt);
      }
    }
  }
  
  TheGlobalSettings.brightness_night = initialNightBrightness;
  
  // Midday
  testTime.hour = 0x12;
  printTime(&testTime);
  
  IncreaseBrightness(&testTime);
  
  if (TheGlobalSettings.brightness_night != initialNightBrightness)
  {
    static const char PROGMEM fmt[]=": Wrong setting changed.\n";
    printf_P(fmt);
    errorOccurred = 1;
  }
  else
  {
    if (TheGlobalSettings.brightness != initialBrightness + 1)
    {
      static const char PROGMEM fmt[]=": Expected %d, got %d\n";
      printf_P(fmt, initialBrightness + 1, TheGlobalSettings.brightness);
      errorOccurred = 1;
    }
    else
    {
      for (int i = 0; i < 15; ++i)
        IncreaseBrightness(&testTime);
        
      if (TheGlobalSettings.brightness != 15)
      {
        static const char PROGMEM fmt[]=": Setting didn't clip, got %d\n";
        printf_P(fmt, TheGlobalSettings.brightness);
        errorOccurred = 1;
      }
      else
      {
        static const char PROGMEM fmt[]=": OK\n";
        printf_P(fmt);
      }
    }
  }
}

void Test_DecreaseBrightness()
{
  static const char PROGMEM title []= "DecreaseBrightness..\n";
  printf_P(title);
        
  const uint8_t initialBrightness = 10, initialNightBrightness = 3;
  
  TheGlobalSettings.brightness = initialBrightness;
  TheGlobalSettings.brightness_night = initialNightBrightness;

  struct DateTime testTime;
  
  // Midnight
  testTime.year = 0x00;
  testTime.month = 0x07;
  testTime.day = 0x01;
  testTime.hour = 0x00;
  testTime.min = 0x00;
  testTime.sec = 0x00;
  
  printTime(&testTime);
  
  DecreaseBrightness(&testTime);
  
  if (TheGlobalSettings.brightness != initialBrightness)
  {
    static const char PROGMEM fmt[]=": Wrong setting changed.\n";
    printf_P(fmt);
    errorOccurred = 1;
  }
  else
  {
    if (TheGlobalSettings.brightness_night != initialNightBrightness - 1)
    {
      static const char PROGMEM fmt[]=": Expected %d, got %d\n";
      printf_P(fmt, initialNightBrightness - 1, TheGlobalSettings.brightness_night);
      errorOccurred = 1;
    }
    else
    {
      for (int i = 0; i < 15; ++i)
        DecreaseBrightness(&testTime);
        
      if (TheGlobalSettings.brightness_night != 0)
      {
        static const char PROGMEM fmt[]=": Setting didn't clip, got %d\n";
        printf_P(fmt, TheGlobalSettings.brightness_night);
        errorOccurred = 1;
      }
      else
      {
        static const char PROGMEM fmt[]=": OK\n";
        printf_P(fmt);
      }
    }
  }
  
  TheGlobalSettings.brightness_night = initialNightBrightness;
  
  // Midday
  testTime.hour = 0x12;
  printTime(&testTime);
  
  DecreaseBrightness(&testTime);
  
  if (TheGlobalSettings.brightness_night != initialNightBrightness)
  {
    static const char PROGMEM fmt[]=": Wrong setting changed.\n";
    printf_P(fmt);
    errorOccurred = 1;
  }
  else
  {
    if (TheGlobalSettings.brightness != initialBrightness - 1)
    {
      static const char PROGMEM fmt[]=": Expected %d, got %d\n";
      printf_P(fmt, initialBrightness - 1, TheGlobalSettings.brightness);
      errorOccurred = 1;
    }
    else
    {
      for (int i = 0; i < 15; ++i)
        DecreaseBrightness(&testTime);
        
      if (TheGlobalSettings.brightness != 0)
      {
        static const char PROGMEM fmt[]=": Setting didn't clip, got %d\n";
        printf_P(fmt, TheGlobalSettings.brightness);
        errorOccurred = 1;
      }
      else
      {
        static const char PROGMEM fmt[]=": OK\n";
        printf_P(fmt);
      }
    }
  }
}

int main()
{ 
  stdout = &mystdout;

  Test_BCDToBin();
  Test_BCDAdd();
  Test_BCDSub();
  Test_HandleEditUp();
  Test_HandleEditDown();

  Test_GetDayOfWeek();
  Test_GetDaysPerMonth();
  Test_GetDateOfLastSunday();
  Test_NormalizeHours();
  Test_IsDSTActive();
  Test_IsItDarkOutside();
  Test_GetActiveBrightness();
  Test_IncreaseBrightness();
  Test_DecreaseBrightness();

  if (errorOccurred)
  {
    static const char PROGMEM fmt[] = "Test done, with errors\n";
    printf_P(fmt);
  }
  else
  {
    static const char PROGMEM fmt[] = "Tests done, no errors\n";
    printf_P(fmt);
  }
  sleep_cpu();  
}


