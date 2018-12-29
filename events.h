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
#ifndef __EVENTS_H__
#define __EVENTS_H__

#define BUTTON1_CLICK 0x0001 
#define BUTTON2_CLICK 0x0002 
#define CLOCK_UPDATE  0x0004 // At 4, because it should mask bit 2.
#define BUTTON3_CLICK 0x0008
#define BUTTON4_CLICK 0x0010
#define BUTTON5_CLICK 0x0020
#define BUTTON6_CLICK 0x0040
#define BUTTON7_CLICK 0x0080
#define BUTTON1_RELEASE 0x0100
#define BUTTON2_RELEASE 0x0200
#define CLOCK_TICK      0x0400
#define BUTTON3_RELEASE 0x0800
#define BUTTON4_RELEASE 0x1000
#define BUTTON5_RELEASE 0x2000
#define BUTTON6_RELEASE 0x4000
#define BUTTON7_RELEASE 0x8000

#endif
