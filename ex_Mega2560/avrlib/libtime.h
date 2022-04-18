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
 *                       emulation. libtime.c *must* be in the compile order.
 *                       AVR Targets : libtime.c is not required.
 * 
 ***************************************************************************/

#ifndef _LIBTIME_H_
#define _LIBTIME_H_

#ifdef EMULATE_LIB
 #include <stdint.h>
 /* --------------------------------------------------------------------
  * tm_delay_ms()
  * Perform a blocking delay for a specific amount of time, in 
  * milliseconds. The delay is a non-negative integer time and not a
  * floating point value (as used in the AVR native delay).
  * ------------------------------------------------------------------*/
 void tm_delay_ms(double delay);
#else
 #define tm_delay_ms  _delay_ms
 #include <util/delay.h>
#endif /* EMULATE_LIB */

#ifdef EMULATE_LIB
 /* --------------------------------------------------------------------
  * tm_delay_us()
  * Perform a blocking delay for a specific amount of time, in 
  * microseconds. The delay is a non-negative integer time and not a
  * floating point value (as used in the AVR native delay).
  * ------------------------------------------------------------------*/
 void tm_delay_us(double delay);
#else
 #define tm_delay_us  _delay_us
#endif /* EMULATE_LIB */

#endif /* _LIBTIME_H_ */
