CC=avr-gcc
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
AVRDUDE=avrdude
MCU=atmega328p
FREQ=16000000
CURRENT_DIR = $(shell pwd)

# For Arduino bootloader
#BAUDRATE=-b 57600
AVRDUDE_FLAGS = -c arduino $(BAUDRATE) -P $(PORT)
PORT=/dev/ttyUSB0

# For UsbTiny ISP
# AVRDUDE_FLAGS = -c usbtiny

SOURCES= main.c font.c Panels.c TimeRenderer.c
A_SOURCES = 
TARGET= PanelClock

ASFLAGS+= -mmcu=$(MCU) -DF_CPU=$(FREQ) -Wa,-gstabs,--listing-cont-lines=100
CFLAGS=-Wall -Os -DF_CPU=$(FREQ)UL -std=c99 -mmcu=$(MCU)  -g -I $(CURRENT_DIR) -I ..

OBJECTS=$(SOURCES:%.c=obj/%.o)
OBJECTS+=$(A_SOURCES:%.S=obj/%.o)

DEPS=$(OBJECTS:.o=.d)

LDFLAGS+= -Wl,-Map=$(TARGET).map

.PHONY: all clean realclean flash outputdir verify
all: $(TARGET).hex

clean: 
	rm -rf $(OBJECTS) $(DEPS) obj/$(TARGET).elf $(TARGET).hex $(TARGET).map	font.c

realclean:  clean
	rm -rf obj

outputdir:
	mkdir -p obj

-include $(DEPS)
font.c:	font.txt
	lua mkfont.lua font.txt > font.c

obj/%.o: %.c
	$(CC) -MMD $(CFLAGS) -c $< -o $@

obj/%.o: %.S
	$(CC) -MMD $(ASFLAGS) -x assembler-with-cpp -c $< -o $@

obj/$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -mmcu=$(MCU) $^ $(LIBS) -o $@

%.hex: obj/%.elf
	$(OBJCOPY) -O ihex -R .eeprom  $< $@

%.lst: %.elf
	$(OBJDUMP) -S -s $< > $@

flash: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -F -V -p $(MCU) -U flash:w:$<

verify: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -F -V -p $(MCU) -U flash:v:$<
