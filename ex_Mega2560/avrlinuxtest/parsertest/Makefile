## IMPORTANT - This compiles AVR code libraries natively in Linux to reapidly test
##             and develop code with needed I/O mocks.


## Project Name (Executable and Binfile name)
NAME := libtest

## LIBRARY SUPPORT (base dir ../avrlib)
VPATH=../../avrlib
LIB_SRC := stringutils.c chardev.c driver.c cmdparser.c
LIB_HDR := stringutils.h chardev.h driver.h ioctlcmds.h cmdparser.h

## Sourcefiles, manually entered
#SOURCES := foo.c bar.c etc...
#HEADERS := foo.h bar.h etc...
##
## -OR- use all .c, .h files found in this Makefile folder + opt. libs folder
SOURCES := $(wildcard *.c) $(LIB_SRC)
HEADERS := $(wildcard *.h) $(LIB_HDR)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

TARGET = $(NAME)
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall -I ../../

.PHONY: default all clean cleanall

default: $(TARGET)
all: default

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o

cleanall:
	-rm -f *.o
	-rm -f $(TARGET)

