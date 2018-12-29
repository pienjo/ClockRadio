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

#ifndef __7SEGMENT_H__
#define __7SEGMENT_H__
#include <stdint.h>
#include <avr/pgmspace.h>

/* The MAX7219 has built-in BCD-to-7-segment translation, but it is designed
   to be used with common cathode displays. In this case, every digit 
   directly corresponds to a register in the MAX7219. Of course, all I had 
   laying about was common anode. While these will work, it means the 
   translation table has to be rotated 90 degrees; the digits now correspond
   to a single bit within every register. 

   To add insult to injury,the small adapter plate was designed to reduce the
   complexity of the connections. As a result, the segments are not in the
   expected order. 
   
   When operated as a 8x8 graphic mode, the 7-segment segments of each digit
   corresponds to the following rows:

   segment	  row         binary
   ---------------------------------
      a		   6         0x20
      b            7         0x40
      c            5         0x10
      d            1         0x01
      e            3         0x04
      f            8         0x80
      g            2         0x02
      d.p.         4         0x08

  The columns are connected as follows (counting from left to right)
    
    Digit #       column
  ------------------------
      1            5
      2            3
      3            2
      4            8

  There are four LEDs. Left one is connected between row 1, column 4. Right
  one is connected between row 2, column 7
    
   row            column
  --------------------
    1		   4
    4              6
    2              1
    2              7
*/

#define DIGIT_1 3
#define DIGIT_2 5
#define DIGIT_3 6
#define DIGIT_4 0 

#define DIGIT_LEFTMOST_LED  4
#define DIGIT_LEFT_LED      2
#define DIGIT_RIGHT_LED     7
#define DIGIT_RIGHTMOST_LED 1

#define SEG_DP 0x10
#define SEG_a  0x04
#define SEG_b  0x02
#define SEG_c  0x08
#define SEG_d  0x80
#define SEG_e  0x20
#define SEG_f  0x01
#define SEG_g  0x40

#define SEG_LED 0xff

// Corresponds to the "Code B" font listed in the MAX7219 datasheet, transposed 
extern const uint8_t PROGMEM BCDToSegment[16];

#endif
