/**********************************************************************
 * avrlib.h
 * GLOBAL header definitions, types and structures.
 * 
 * This header should be referenced instead of <stdint.h>  and the like.
 * It will define common types specific to avrlib as well, eg. memory 
 * pointer types.
 * 
 * Version 1.0
 *
 **********************************************************************/

#ifndef _AVRLIB_H_
#define _AVRLIB_H_

#include <stdint.h> 
#include <stddef.h>  /* NULL */

// Atmel Internal register pointer types
typedef volatile uint8_t *  sfr8p_t;
typedef volatile uint16_t * sfr16p_t;
typedef volatile uint32_t * sfr32p_t;


#endif /* _AVRLIB_H_ */

