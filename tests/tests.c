
#include <avr/io.h>
#include <stdio.h>
#include <avr/sleep.h>

#include <simavr/avr/avr_mcu_section.h>
#include "../Timefuncs.h"
#include "../DateTime.h"
#include "../BCDFuncs.h"

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
  printf("GetDayOfWeek..\n");
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
      printf("%02x/%02x/20%02x: Expected %2x, got %2x\n", day, month, year, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      printf("%02x/%02x/20%02x: OK (%d)\n", day, month, year, expect);
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
  printf("GetDayPerMonth..\n");
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
      printf("%02x/20%02x: Expected %02x, got %02x\n",month, year, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      printf("%02x/20%02x: OK (%02x)\n",month, year, expect);
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
  printf("GetDateOfLastSunday..\n");
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
      printf("%02x/20%02x: Expected %02x, got %02x\n",month, year, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      printf("%02x/20%02x: OK (%02x)\n",month, year, expect);
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
  printf("BCDToBin..\n");
  
  for (int testIdx = 0;; ++testIdx)
  {
    const uint8_t input = pgm_read_byte(2 * testIdx + BCDToBin_tests + 0),
		  expect = pgm_read_byte(2 * testIdx + BCDToBin_tests + 1);
		  
    if (input == 0)
      break;
      
    const uint8_t actual = BCDToBin(input);
    
    if (actual != expect)
    {
      printf("0x%02x: Expected %d, got %d\n", input, expect, actual);
      errorOccurred = 1;
    }
    else 
    { 
      printf("0x%02x: OK (%d)\n", input, actual);
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
  printf("BCDAdd..\n");
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
      printf("0x%02x + 0x%02x: Expected 0x%02x, got 0x%02x\n", left, right, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      printf("0x%02x + 0x%02x: OK(0x%02x)\n", left, right, actual);
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
  printf("BCDSub..\n");
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
      printf("0x%02x - 0x%02x: Expected 0x%02x, got 0x%02x\n", left, right, expect, actual);
      errorOccurred = 1;
    }
    else
    {
      printf("0x%02x - 0x%02x: OK(0x%02x)\n", left, right, actual);
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
  0x00
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
  printf("%02x/%02x/%02x %02x:%02x:%02x",time->year, time->month, time->day, time->hour, time->min, time->sec); 
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
  
  printf("NormalizeHours..\n");
  for( int testIdx = 0; ; ++testIdx)
  {
    testTime.year = pgm_read_byte( 8 * testIdx + NormalizeHours_tests + 0);
    if (testTime.year == 0)
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
       printf(" OK: (");
       printTime(&testTime);
       printf(")\n");
    }
    else
    {
      printf(": Expected ");
      printTime(&expectedTime);
      printf(" Got ");
      printTime(&testTime);
      uart_putchar('\n', stdout);
      errorOccurred =1;
    }
    
  }
}

int main()
{ 
  stdout = &mystdout;

  Test_BCDToBin();
  Test_BCDAdd();
  Test_BCDSub();
  Test_GetDayOfWeek();
  Test_GetDaysPerMonth();
  Test_GetDateOfLastSunday();
  Test_NormalizeHours();
  
  if (errorOccurred)
    printf("Test done, with errors\n");
  else
    printf("Tests done, no errors\n");
  sleep_cpu();  
}


