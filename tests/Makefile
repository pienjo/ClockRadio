CC=avr-gcc
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
MCU=atmega168p
FREQ=16000000
CURRENT_DIR = $(shell pwd)

SOURCES= tests.c ../Timefuncs.c ../BCDFuncs.c ../settings.c
TARGET= PanelClock_test

ASFLAGS+= 
CFLAGS=-Wall -Os -DF_CPU=$(FREQ)UL -std=c99 -mmcu=$(MCU)  -fno-inline-small-functions -ffunction-sections -fdata-sections -Wl,--relax,--gc-sections -Wl,--undefined=_mmcu,--section-start=.mmcu=0x910000 -I/usr/include/simavr

OBJECTS=$(SOURCES:%.c=obj/%.o)

.PHONY: all clean outputdir run

all: run

clean: 
	rm -rf $(OBJECTS) $(TARGET).axf $(TARGET).map

realclean:  clean
	rm -rf obj

outputdir:
	mkdir -p obj

run: $(TARGET).axf
	simavr $(TARGET).axf

obj/%.o: %.c
	$(CC) -gdwarf-2 $(CFLAGS) -c $< -o $@

$(TARGET).axf: $(OBJECTS)
	$(CC) -gdwarf-2 $(CFLAGS) $^ $(LIBS) -o $@ -Wl,-Map=$(TARGET).map


