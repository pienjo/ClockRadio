This is just a simple alarm clock.

How to use
==========

## Main display

A short press on the mode button rotates between the following modes:

  * Time. The first 2 columns of the LED display are used for a Day-of-week indicator; monday is at the first row.
  * Date
  * Alarm 1.  
  * Alarm 2. 
  * One-time alarm

A long press on the mode button will edit whatever is being displayed (time,
alarm 1, alarm 2, or one-time alarm). Performing a long press while the
date is being shown will edit the time drift compensation. The display will
return to ''Time'' when no buttons are pressed.

## Editing

A short press on the mode button will advance to the next digit/number. Use 
the increment/decrement buttons to change digits/numbers. A long press
on the mode button will abort the edit.

## Radio

Pressing the on/off button will toggle the radio. When the radio is active,
the "Time" page will show the tuned frequency instead. When the radio is on,
the "Increase" and "Decrease" can be used for tuning. A short press on either
button will do a manual tune, a long press will perform an automatic seek for
the next channel.

To change the volume of the radio, hold down the on/off button and use the
increase/decrease buttons for volume control.

## Alarm

There are 2 regular alarms. They can sound on weekdays, on weekends, or both. 
They can be edited by doing a long-press on the mode button while the relevant
alarm is being displayed.

To disable or enable an alarm, press the ''on/off'' button while the alarm is
being displayed. Press the ''increment'' button to silence only the upcoming
invocation of the alarm. I added this because I often wake up before the
alarm is set to sound; in these cases I want to disable the alarm for today, 
but it should still arm itself for tomorrow without me having to remember
doing so...

Alarms can either turn on the radio, or turn on the beeper. While an alarm is
active, any button can be used to disable the alarm (the regular function of
the buttons is disabled while an alarm is active). When an alarm is scheduled
to go off while the previous alarm is still active, the first alarm is 
automatically silenced. (for example: my first alarm turns on the radio at
6:55 so I can listen to the news. If I'm not out of bed by 7:10, the second
alarm will sound the beeper).

Alarms will automatically be disabled after a certain timeout (1 hour for
radio alarms, 4 minutes for beeper alarms).

There is also a one-time alarm, for those times where you need to wakeup
at a non-standard hour. It behaves the same as the normal alarms, but doesn't
re-arm itself.

## sleep/nap

There is a sleep- or naptimer, sharing a single button. When the radio is
turned on while the sleep/nap button is pressed, the sleep function activates.
Press the button repeatedly to change the desired sleep interval. The radio
will turn off after this time.

If the radio was turned off when the sleep/nap button was pressed, the nap
function activates. It will sound the beeper alarm once the nap timer has 
expred. Press the sleep/nap button repeatedly to change the desired nap 
interval.

Both sleep and nap timers have a '':00'' interval - Selecting this interval
will disable the sleep/nap timer.

Hardware
========

Unfortunately, I lost the schematics for this clock. It was kind of cobbled
together at the spot. I'll include an overview, which should be enough to 
recreate a suitable schematic.

## Buttons

There are 5 switches, which pull down to GND:
 
  * **mode** (''BUTTON_1'', connected to PD0)
  * **on/off** (''BUTTON_2'', connected to PD1) 
  * **decrease** (''BUTTON_3'', connected to PD3)
  * **increase** (''BUTTON_4'', connected to PD4)
  * **sleep/nap** (''BUTTON_5'', connected to PD5)

There is **no** button connected to PD2 (ex0). Instead, it is connected to the
clock output of the DS1307. This will generate ''CLOCK_UPDATE'' events.

## Display 

The display consists of 4 MAX7219 modules from AliExpress. They came with an
8x8 LED matrix, and are connected in cascade (DOUT connected to DIN of the next
module. SCK is shared among all modules). 

They are configured over SPI, and use the hardware SPI module of the AVR:

  * MOSI (PB3 of the AVR) is connected to DIN of the first module.
  * SCK (PB5 of the AVR) is connected to CLK of the first module.
  * PB2 is connected to LOAD(/CS) of the first module.

I replaced the 8x8 matrix of the last module with a 4x7 segment display
plus some status LEDs on an adapter board. See the "schematics" in 
Hardware/display ) Note that the 4x7 segment that I happened to find in my
parts bin was common anode rather than common cathode!

## FM Radio

The clock uses an SI4702 radio module. **This module must be supplied with
3.3V**. I used an XC6206 low-dropout regulator.

The module is configured over I2C, using the hardware I2C module (SDA at PC4,
SCL at PC5). These need to be level-shifted because of difference in supply
voltage - I used a standard BSS138 based level shifter. See 
Hardware/levelshifter .

In addition, the reset line of the SI4702 module is connected to PC3. It 
doesn't need a level shifter; the AVR will only pull this low during the
power-up of the FM radio.

The outputs of the SI4702 need to be decoupled using a suitably large 
capacitor (I think I used a 10 uF electrolytic) because of different biasing.

## Beeper 

PC1 is used to generate a crude beeper. It is connected to the input of the
audio amplifier, using a potentiometer (for volume control) and a small
decoupling capacitor.

## Audio amplifier

I used a cheap amplifier module from AliExpress (LM386-based), connected to
4 small (29mm) speakers, .25W @ 8 Ohm, connected in two parallel chains of
2 (for a total impedance of 8 Ohm).

Since this amplifier turned out to be rather noisy, I added a PNP transistor
to its supply line, so it can be turned off if the radio and beeper are 
quiet. This transistor is controlled by PC2.

Building
========

You need to have the following installed:

  * A recent version of gcc-avr 
  * A recent version of avr-libc
  * avrdude for programming. The TX and RX lines are in use, so it's
    probably best to use an ISP programmer.
  * lua (5.1 or 5.2) to generate the bitmap- and font code.

To run the unittests in tests, you'll also need simavr plus its headers.

Case
====

I posted the case I made for this alarm clock to Thingiverse, see 

https://www.thingiverse.com/thing:3313125
