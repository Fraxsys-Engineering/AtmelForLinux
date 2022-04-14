/****************************************************************************
 * stringutils.h
 * Misc. string, ASCII-HEX and number conversion utility functions.
 * 
 * Version 1.0
 *
 ***************************************************************************/

#ifndef _STRINGUTILS_H_
#define _STRINGUTILS_H_

#include <stdint.h>  // uint8_t etc.

#define SU_HI_NIBBLE 	1
#define SU_LO_NIBBLE 	0

#define SU_UPPERCASE 	1
#define SU_LOWERCASE 	0

#define SU_USE_PREFIX 	1
#define SU_NO_PREFIX 	0

// Calculate string lengths
int32_t sutil_strlen(const char * s);

// copy strings
int32_t sutil_strcpy(char * to, char * from);

// copy mem
int32_t sutil_memcpy(void * to, void * from, int len);

// Simple string token parsing.
// Given a string buffer 'b', find the first occurance matching any 
// character in buffer 'm'. The found token is replaced by a EOL '\0'.
// The function returns the pointer to the next char in 'b' beyond the
// replaced character to allow for further parsing.
// If nothing is found then NULL is returned.
// If at the end of 'b' then the returning pointer will be the '\0' 
// found at the end of 'b'.
char * sutil_strtok(char * b, const char * m);

// string compare (simple)
int sutil_strcmp(const char * a, const char * b);

// sutil_nibble()
// convert the high or low nibble of a byte value in 'n' into its 
// corresponding ASCII-HEX character (0-9,A-F|a-f).
// Uses upper-case (A-F) if 'uc' is TRUE, else returns (a-f)
// -
// Returns: converted ASCII-HEX character.
char sutil_nibble(uint8_t n, uint8_t isHiNibble, uint8_t uc);

// sutil_asciihex_byte()
// Generate and insert a single byte (2-nibble) ASCII-HEX string into 
// the given string buffer 'at'. The given string buffer must be at
// least 2 bytes (no prefix) or 4 bytes (w/ prefix) of space left.
// - 
// Place ASCII-HEX string value of 'hval' at pointer 'at' and 'at'+1,
// or at 'at'+2,3 and prefix string 'at' with "0x"
// For uppercase (A-F) in the string, set 'uc' to TRUE.
// - 
// return # bytes advanced into 'at'
uint8_t sutil_asciihex_byte(char * at, uint8_t hval, uint8_t prefix, uint8_t uc);

// sutil_asciihex_word()
// Same as sutil_asciihex_byte() but generates a 2-byte (4 nibble) 
// string. Needs 4 or 6 bytes of free space in 'at' buffer.
// - 
// return # bytes advanced into 'at'
uint8_t sutil_asciihex_word(char * at, uint16_t hval, uint8_t prefix, uint8_t uc);

// sutil_asciihex_long()
// Same as sutil_asciihex_byte() but generates a 4-byte (8 nibble) 
// string. Needs 8 or 10 bytes of free space in 'at' buffer.
// - 
// return # bytes advanced into 'at'
uint8_t sutil_asciihex_long(char * at, uint32_t hval, uint8_t prefix, uint8_t uc);

// sutil_asciinumber()
// Encode a given integer value (positive or negative) into a given string
// in base10.
// (!) Given buffer must be at least 12 bytes, to cover +/- max ranges of 
//     a 32-bit integer.
// - 
// return # bytes advanced into 'at'
uint8_t sutil_asciinumber(char * at, int32_t ival);


#endif /* _STRINGUTILS_H_ */
