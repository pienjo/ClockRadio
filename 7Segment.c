#include "7Segment.h"

// Corresponds to the "Code B" font listed in the MAX7219 datasheet, transposed 
const uint8_t PROGMEM BCDToSegment[16]=
{
  (SEG_a + SEG_b + SEG_c + SEG_d + SEG_e + SEG_f),  // 0: abcdef_
  (SEG_b + SEG_c),                                  // 1: _bc____
  (SEG_a + SEG_b + SEG_d + SEG_e + SEG_g),          // 2: ab_de_g
  (SEG_a + SEG_b + SEG_c + SEG_d + SEG_g),          // 3: abcd__g
  (SEG_b + SEG_c + SEG_f + SEG_g),                  // 4:_bc__fg
  (SEG_a + SEG_c + SEG_d + SEG_f + SEG_g),          // 5: a_cd_fg
  (SEG_a + SEG_c + SEG_d + SEG_e + SEG_f + SEG_g),  // 6: a_cdefg
  (SEG_a + SEG_b + SEG_c),                          // 7: abc____
  ~(SEG_DP),					    // 8: abcdefg
  (SEG_a + SEG_b + SEG_c + SEG_d + SEG_f + SEG_g),  // 9: abcd_fg
  SEG_g,					    // _: ______g
  (SEG_a + SEG_d + SEG_e + SEG_f + SEG_g),          // E: a__defg
  (SEG_b + SEG_c + SEG_e + SEG_f + SEG_g),          // H: _bc_efg
  (SEG_d + SEG_e + SEG_f),			    // L: ___def__
  (SEG_a + SEG_b + SEG_e + SEG_f + SEG_g),          // P: ab__efg
  0,                                                //  : _______
};

