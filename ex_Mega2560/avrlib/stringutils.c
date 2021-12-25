/****************************************************************************
 * stringutils.c
 * Misc. string, ASCII-HEX and number conversion utility functions.
 * 
 * Version 1.0
 *
 ***************************************************************************/

#include "stringutils.h"

#define HI_NIB 1
#define LO_NIB 0
#define AH_PREFIX 1
#define NO_PREFIX 0

char sutil_nibble(uint8_t n, uint8_t isHiNibble, uint8_t uc) {
	static char nbchar_uc[] = "0123456789ABCDEF";
	static char nbchar_lc[] = "0123456789abcdef";
	n = (isHiNibble)
		? (n >> 4) & 0x0f
		: n & 0x0f;
	return (uc) ? nbchar_uc[n] : nbchar_lc[n];
}

uint8_t sutil_asciihex_byte(char * at, uint8_t hval, uint8_t prefix, uint8_t uc) {
	uint8_t ptr = 0;
	if (prefix) {
		at[ptr++] = '0';
		at[ptr++] = (uc) ? 'X' : 'x';
	}
	at[ptr++] = sutil_nibble(hval, HI_NIB, uc);
	at[ptr++] = sutil_nibble(hval, LO_NIB, uc);
	return ptr;
}

uint8_t sutil_asciihex_word(char * at, uint16_t hval, uint8_t prefix, uint8_t uc) {
	uint8_t rc = sutil_asciihex_byte(at, ((hval >> 8) & 0xff), prefix, uc);
	rc += sutil_asciihex_byte(at+rc, (hval & 0xff), 0, uc);
	return rc;
}

uint8_t sutil_asciihex_long(char * at, uint32_t hval, uint8_t prefix, uint8_t uc) {
	uint8_t rc = sutil_asciihex_word(at, ((hval >> 16) & 0xffff), prefix, uc);
	rc += sutil_asciihex_word(at+rc, (hval & 0xffff), 0, uc);
	return rc;
}


