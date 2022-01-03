/****************************************************************************
 * libtime.h
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

#ifndef _LIBTIME_H_
#define _LIBTIME_H_

#include <stdint.h>

// move to *.c and only when not emulated.
//#include <avr/io.h>
//#include <util/delay.h>

/* --------------------------------------------------------------------
 * tm_delay_ms()
 * Perform a blocking delay for a specific amount of time, in 
 * milliseconds. The delay is a non-negative integer time and not a
 * floating point value (as used in the AVR native delay).
 * ------------------------------------------------------------------*/
void tm_delay_ms(uint32_t delay);


#endif /* _LIBTIME_H_ */

