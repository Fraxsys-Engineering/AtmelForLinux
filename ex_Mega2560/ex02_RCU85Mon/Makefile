## Project Name (Executable and Binfile name)
NAME := rcu85mon

## Sourcefiles, manually entered
#SOURCES := foo.c bar.c etc...
#HEADERS := foo.h bar.h etc...
##
## -OR- use all .c, .h files found in this Makefile folder
SOURCES := $(wildcard *.c)
HEADERS := $(wildcard *.h)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

## Atmel AT328P ---------
#MCU := atmega328p
#AVR_MCU := ATMEGA328P
#AVRD_FLAGS := -F -V
##
## Atmel "Mega" 2560 ----
MCU := atmega2560
AVR_MCU := m2560
AVRD_FLAGS := -F -V -D -v

## Programmer - DEFAULT (As wired)
## RECOMMENDED - Leave this programmer enabled if using
## a target board with USB integrated onboard. This one
## usually works.
PARTNO := wiring
#
## Programmer - ST-Link/V2 -----
#PARTNO := stk500v2
#
## Programmer - Arduino ------
## Arduino BOOTLOADER is already loaded on device
## Target Board has a USB connection
#PARTNO := arduino
#
## Programmer - Arduino/FT232 -
## Arduino BOOTLOADER is already loaded on device
## Target Board has a FT232R USB/Serial Dongle attached - no onboard USB
#PARTNO := arduino-ft232r

## Mounted Serial Port TTY, for connection to board
## (check for new /dev/tty* entries after plugging board in)
SERDEV := /dev/ttyACM0

## Baudrate, usually 115200
BAUD := 115200

## Target CPU Clock, usually 16 MHz
CPU_CLK := 16000000UL

CFLAGS := -Wall -pedantic

## Compiling for Size (Default)
CFLAGS += -Os
#
## Compiling for Debugging
#CFLAGS += -O0 -g

CFLAGS += -std=c99 -mmcu=$(MCU) -DF_CPU=$(CPU_CLK)

CC := avr-gcc
OBJCOPY := avr-objcopy
SIZE := avr-size -A
AVRD := avrdude

HEX := $(NAME).hex
OUT := $(NAME).out
MAP := $(NAME).map


all: $(HEX)

.PHONY: clean
clean:
	rm -vf *.o *.out *.map

.PHONY: cleanall
cleanall:
	rm -vf *.o *.out *.map *.hex

.PHONY: readfuses
readfuses:
	$(AVRD) -c $(PARTNO) -p $(AVR_MCU) -P $(SERDEV) -b $(BAUD) -U lfuse:r:low_fuse.hex:h -U hfuse:r:high_fuse.hex:h -U efuse:r:ext_fuse.hex:h
	rm low_fuse.hex high_fuse.hex ext_fuse.hex
	
download: $(HEX)
	$(AVRD) $(AVRD_FLAGS) -c $(PARTNO) -p $(AVR_MCU) -P $(SERDEV) -b $(BAUD) -U flash:w:$(HEX)

$(HEX): $(OUT)
	$(OBJCOPY) -R .eeprom -O ihex $< $@

$(OUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ -Wl,-Map,$(MAP) $^
	@echo
	@$(SIZE) $@
	@echo

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

%.pp: %.c
	$(CC) $(CFLAGS) -E -o $@ $<

%.ppo: %.c
	$(CC) $(CFLAGS) -E $<


