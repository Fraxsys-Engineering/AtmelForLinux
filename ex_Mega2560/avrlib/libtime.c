/****************************************************************************
 * libtime.c
 * API for time-based operations.
 * 
 * Version 1.0
 *
 * DO NOT CALL NATIVE AVR C functions from eg. time.h. Use this API
 * to get an adaptation layer and additional custom functions. This API
 * can be emulated in Linux.
 * 
 * OPTIONAL DEFINITIONS
 *
 *  EMULATE_LIB          If defined then the underlying OS time functions
 *                       are moved to Linux OS based ones, used for target
 *                       emulation
 * 
 ***************************************************************************/


#ifdef EMULATE_LIB

#include <unistd.h>
#include "libtime.h"

/* --------------------------------------------------------------------
 * tm_delay_ms()
 * Perform a blocking delay for a specific amount of time, in 
 * milliseconds. The delay is a non-negative integer time and not a
 * floating point value (as used in the AVR native delay).
 * ------------------------------------------------------------------*/
void tm_delay_ms(double delay) {
    useconds_t udelay = (useconds_t)(delay * 1000.0);
    usleep(udelay);
}

#endif /* EMULATE_LIB */


