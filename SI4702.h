#ifndef __SI4702_H__
#define __SI4702_H__

void Init_SI4702();
void Poll_SI4702();
void SI4702_Seek(_Bool seekUp);
uint16_t SI4702_GetFrequency();
void SI4702_SetFrequency(uint16_t frequency);
void SI4702_SetVolume(uint8_t volume);
void SI4702_PowerOn();
void SI4702_PowerOff();
#endif

