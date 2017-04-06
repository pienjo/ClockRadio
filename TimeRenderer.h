#include <stdbool.h>
bool TimeRenderer_Tick(); // Must be called regularly for animations - until it returns false

// Set the time to be displayed. if _animate is true, use "rolling" animation
bool TimeRenderer_SetTime(uint8_t hour, uint8_t minutes, uint8_t seconds, bool animate);
