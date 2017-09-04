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

  There are two LEDs. Left one is connected between row 1, column 4. Right
  one is connected between row 2, column 7
*/

#define DIGIT_1 3
#define DIGIT_2 5
#define DIGIT_3 6
#define DIGIT_4 0 

#define DIGIT_LEFT_LED 1
#define DIGIT_RIGHT_LED 4

#define SEG_DP 0x10
#define SEG_a  0x04
#define SEG_b  0x02
#define SEG_c  0x08
#define SEG_d  0x80
#define SEG_e  0x20
#define SEG_f  0x01
#define SEG_g  0x40

#define SEG_LEFT_LED 0xff
#define SEG_RIGHT_LED 0xff

// Corresponds to the "Code B" font listed in the MAX7219 datasheet, transposed 
extern const uint8_t PROGMEM BCDToSegment[16];

#endif
