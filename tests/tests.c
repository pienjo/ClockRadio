
#include <avr/io.h>
#include <stdio.h>
#include <avr/sleep.h>

#include <simavr/avr/avr_mcu_section.h>
#include "../Timefuncs.h"
#include "../DateTime.h"
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

const uint8_t PROGMEM DOW_tests[] = 
{
  // day, month, year, expected
  1, 12, 0x00, 5,
  1, 5, 0x17, 1,
  1, 1, 0x18, 1,
  2, 1, 0x18, 2,
  3, 1, 0x18, 3,
  4, 1, 0x18, 4,
  5, 1, 0x18, 5,
  6, 1, 0x18, 6,
  7, 1, 0x18, 7,
  8, 1, 0x18, 1,
  31,1, 0x18, 3,
  1, 2, 0x18, 4,
  6, 4, 0x18, 5, 
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

    const uint8_t readable_year = 10* (year >> 4) +  (year & 0xf);
    if (actual != expect)
    {
      printf("%d/%d/20%02d: Expected %d, got %d\n", day, month, readable_year, expect, actual);
    }
    else
    {
      printf("%d/%d/20%02d: OK (%d)\n", day, month, readable_year, expect);
    }

  }
}

const uint8_t PROGMEM DPM_tests[] =  {
  /* Month, year (BCD), expcted */
  1, 1, 0x31,
  2, 1, 0x28,
  3, 1, 0x31,
  4, 1, 0x30,
  5, 1, 0x31,
  6, 1, 0x30,
  7, 1, 0x31,
  8, 1, 0x31,
  9, 1, 0x30,
  10, 1, 0x31,
  11, 1, 0x30,
  12, 1, 0x31,
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
    const uint8_t readable_year = 10* (year >> 4) +  (year & 0xf);
    if (actual != expect)
    {
      printf("%d/20%02d: Expected %d, got %d\n",month, readable_year, expect, actual);
    }
    else
    {
      printf("%d/20%02d: OK (%d)\n",month, readable_year, expect);
    }

  }
}

int main()
{ 
  stdout = &mystdout;

  Test_GetDayOfWeek();
  Test_GetDaysPerMonth();
  printf("Test done\n");
  sleep_cpu();  
}

