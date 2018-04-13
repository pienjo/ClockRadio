#ifndef __SI4702_H__
#define __SI4702_H__

_Bool Init_SI4702();
_Bool Poll_SI4702();
void SI4702_Seek(_Bool seekUp);
void SI4702_Tune(_Bool tuneUp);
uint16_t SI4702_GetFrequency();
void SI4702_SetFrequency(uint16_t frequency);
void SI4702_SetVolume(uint8_t volume);
uint8_t SI4702_GetVolume();
_Bool SI4702_PowerOn();
void SI4702_PowerOff();
#endif

