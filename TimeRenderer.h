#include <stdbool.h>
void TimeRenderer_Tick(); // Must be called regularly for animations

// Set the time to be displayed. if _animate is true, use "rolling" animation
void TimeRenderer_SetTime(uint8_t hour, uint8_t minutes, _Bool animate);
