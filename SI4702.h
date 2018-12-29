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

