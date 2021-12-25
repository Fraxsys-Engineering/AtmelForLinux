Driver API & Stack Information

This text discusses how the avrlib is organized and how the driver 
stack works.

Version: 1.0


[1] Introduction

avrlib is a C code library for supporting "bare iron" user applications
written for the ATMEL ATmega8/8L series MCUs (see compatibility table below). 

 [1.1] LICENSE 
 
This code is made freely available under the GPL v2 licence (see LICENSE file
included in this git repo). 

 [1.2] CONTRIBUTION

If there is intention to extend this library
by others then I would request that these persons *fork* the git repo and
generate pull requests for a code review. The request should identify the 
extension and the supported MCUs along with any ATDD source as appropriate.
Upon approval, the addition will be added to this repository for others 
to use.

 [1.3] Supported MCUs (ATmega8/8L series)

This list is expected to increase over time. At present I am developing 
the library for use with the ATmega2560 MCU so I expect minimal issues 
for other MCUs in the same family line (640, 1280, 1281, 2561).

All MCUs in this list have been fully qualified with all library 
components, unless a specific component is only applicable to one MCU 
part (it is not compiled in for other MCU part numbers in that case).

 - ATmega2560
 



[2] APIs

Generic APIs offered in the library. This does not include the character
stream driver stack, refer to section 3 for that info.

 [2.1] string utils

Location: avrlib/stringutils.h

Utility functions for working on character strings (buffers). This can be
formatting work or converting integers into ASCII-HEX strings for bytes,
words and longs.
 
 [2.2] dblink
 
Location: avrlib/dblink.h

A debugging support library which makes use of onboard LEDs on small 
ATMEL "Arduino" boards, eg. ATMEGA2560. Various routines will blick the
LED in a pattern and either return or infinitely look and blink; useful
as a trap function when something goes wrong.

 [2.3] cmdparser

Location: avrlib/cmdparser.h

This code contains the API and hooks for a simple command parser. The 
user application must extend this code to add their own commands and 
parse the arriving sentences. The code in cmdparser is there to create
a starting point for parsing commands.

The parsing model follows a sentence syntax containing an initial 'noun'
followed by one or more 'verbs' that extend the noun. Nouns and verbs
must be easily parsable and use of static 'key' strings is encouraged.

The user application declares each command in a special struct 
(struct cmdobj) and specifies the noun (exact match of a long-form
noun or an optional shortened form) along with the minimum and maximum 
expected number of verbs. Verbs are *not* matched by the parser and so
they can be any format, including numbers. The command parser must 
parse the verbs in its own way.

Commands are terminated by a carriage-return (and optional line-feed).
Trigger: cr (0x0D)
Any line feeds (0x0A) following the cr are pulled out of the Rx buffer 
and discarded.


[3] Driver Stack

The driver stack is a design for character stream devices with adjacent
control interface, AKA the ioctl API. Some drivers may rely heavily on
the control API; these are pseudo character streams, the i2c device is
and example of this as data is heavily scoped and framed by pre-set 
conditions, eg. device address, which cannot be put in-line in the data
stream itself.

 [3.1] User Space

User space contains headers to allow "user" applications access to use
of the drivers for communications purposes. Nothing un-necessary to the 
"user" is exposed in the user space, instead it is declared in the 
driver space.

APIs:

  [3.1.1] chardev

Location: avrlib/chardev.h

chardev API provides the highest level of abstraction for the char stream
driver stack. Access to a driver is requested, by name, in the open()
call. It is then under control of the user until closed, returning it
to the static pool of available drivers.

  [3.1.2] ioctlcmds

Location: avrlib/ioctlcmds.h

This header file contains all ioctl commands available to the user, by
driver type. All driver types have their commands here but the driver
itself may not be configured in the driver pool!

 [3.2] Driver Space

Driver space contains headers for use by the internal driver stack and 
individual drivers. All driver and stack source code are also in this
space. "User" applications should never have to reference any of these
headers or source code!

Drivers will have one or more instances of themselves made available. The
count is dependant on the MCU type (different MCU's have differing counts
of serial device peripherals, eg. 2 UARTs on one type vs 4 UARTs on 
another). One 'parent' driver is written in code and contains knowledge
of all its peripheral's locations in memory, I/O pins etc. Each peripheral
for that driver is given a child number; this number is declared in the 
driver name when it is opened (ref. chardev.h). The management method of 
children is left up to each driver to impliment in its own way but the
driver prototype (driver.h) contains provisions for caching child info
in the driver 'class' (struct driverhandle_type) in the form of an 
opaque data pointer.

  [3.2.1] driver

Location: avrlib/driver.h, driver.c

'driver' defines a prototype serial character stream driver. The C way
of declaring a base classification of methods is to define function
types and then a struct holding pointers to these types along with some
"public" data and opaque (hidden) data pointers, used by each driver in
its own way.

'driver' also contains the exposed methods for registering and accessing
drivers within a static driver pool. 

   [3.2.2] loopback driver
   
Location: ex_Mega2560/avrlinuxtest/loopback_driver.c

This is a test driver, for compiling a TDD target and driving code 
methods for testing purposes on a linux target. There are no hardware 
peripherals needed for this driver and so it can be rapidly compiled and 
tested in a Linux environment. It has limited usefulness for an AVR 
project, though later it could be expanded to create a pipe for parallel 
applications, if some threading model is ever introduced in future.

   [3.2.3] serial driver
   
Location: avrlib/serialdriver.c

This is the first real driver written, for the ATMEL MCU serial 
peripherals. The number of child instances generated for this driver 
depends on the MCU type being compiled against.

 [3.3] Hierarchy and Dependancies
 
ASCII Art showing code interdependancy


[USER]
  
    +------H------+     +------H------+
    |   chardev   |<----|  ioctlcmds  |
    +-------------+     +-------------+
           |                   |
[DRIVER]   |                   v
           |            +------H------+    +=====H/C=====+
           |            |   driver    |    | ((msc src)) |
           v            +-------------+    +=============+
    +------C------+            |                      |
    |   chardev   |<-----------+--------------+       |
    +-------------+            |              |       |
                               v              v       v
                        +------C------+    +======C======+
                        |   driver    |    |  ((driver)) |
                        +-------------+    +=============+
                        
((msc src)) - (OPTIONAL) some drivers may use external headers and 
source (files outside of the driver stack) for lower level APIs into 
hardware peripherals or perform other processing that is specific to 
that driver.

((driver)) - these are all the character device drivers developed for
operation in this stack. 


