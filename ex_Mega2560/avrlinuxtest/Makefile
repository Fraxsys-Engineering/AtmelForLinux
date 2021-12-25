## IMPORTANT - This compiles AVR code libraries natively in Linux to reapidly test
##             and develop code with needed I/O mocks.


## SECTION -A- ------------------------------------------------------
## Add new tests, prefix: TEST_ 
## Similarly, add new SRC_*, HDR_*, OBJ_* and add to the existing lists,
## eg. ALL_TESTS, HEADERS, .PHONY
## Then create new compile and run directives for the test.
## Don't forget to create a new test suite, use test_stringutils.c as a 
## template.
TEST_stringutils := test_stringutils
TEST_chardriverstack := test_chardriverstack


## SECTION -B- ------------------------------------------------------
## mention all test suites to be bundled together when running 'make all'
ALL_TESTS := $(TEST_stringutils) $(TEST_chardriverstack)


## LIBRARY SUPPORT (base dir ../avrlib)
VPATH=../avrlib


# SECTION -C- -------------------------------------------------------
## Test Suite: stringutils
SRC_stringutils := $(TEST_stringutils).c stringutils.c
HDR_stringutils := stringutils.h
OBJ_stringutils := $(patsubst %.c,%.o,$(SRC_stringutils))

SRC_chardriverstack := $(TEST_chardriverstack).c chardev.c driver.c loopback_driver.c
HDR_chardriverstack := chardev.h driver.h
OBJ_chardriverstack := $(patsubst %.c,%.o,$(SRC_chardriverstack))


# SECTION -D- -------------------------------------------------------
# Concat all header dependancies together. it only means that *any* header
# change will force a re-build of all test regardless of whether the test
# uses that header or not... 
HEADERS := $(HDR_stringutils) $(HDR_chardriverstack)


LIBS = -lm -lcunit
CC = gcc
CFLAGS = -g -Wall -DTDD_PRINTF -I..

.PHONY: default all clean cleanall run_all

default: $(ALL_TESTS)
all: default

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


run_$(TEST_stringutils):
	./$(TEST_stringutils)

run_$(TEST_chardriverstack):
	./$(TEST_chardriverstack)


run_all: run_$(TEST_stringutils) run_$(TEST_chardriverstack)

clean:
	-rm -f *.o

cleanall:
	-rm -f *.o
	-rm -f $(ALL_TESTS)

## static expansion of any test target that follows these rules:
## - target name starts with: test_<testname>
## - object intermediate files are defined in: OBJ_<testname>

.SECONDEXPANSION:

test_%: $$(OBJ_$$*)
	$(CC) $^ -Wall $(LIBS) -o $@


