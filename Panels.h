#ifndef __PANELS_H__
#define __PANELS_H__
#include <stdint.h>

extern uint8_t *panelBitMask;

void InitializePanels(uint8_t numPanels);
void SetBrightness(uint8_t level);
void SendRow(uint8_t row, const uint8_t *data);

#endif
