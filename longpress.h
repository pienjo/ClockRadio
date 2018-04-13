#ifndef __LONGPRESS_H__
#define __LONGPRESS_H__
#include <inttypes.h>

struct longPressResult {
  uint8_t shortPress;
  uint8_t longPress;
  uint8_t repPress;
};

void GetLongPress(const uint16_t events, struct longPressResult *result);
void MarkLongPressHandled(const uint8_t mask);

#endif
