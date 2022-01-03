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

#define NULL ((void *)0)

int32_t sutil_strlen(const char * s) {
    int32_t len = 0;
    while (*s != '\0') {
        len ++;
        s++;
    }
    return len;
}

int32_t sutil_strcpy(char * to, char * from) {
    int fp = 0;
    int flen = sutil_strlen(from);
    while (flen) {
        to[fp] = from[fp];
        fp ++;
        flen --;
    }
    to[fp] = '\0';
    return fp;
}

//#include <stdio.h>
// Simple string token parsing.
// Given a string buffer 'b', find the first occurance matching any 
// character in buffer 'm'. The found token is replaced by a EOL '\0'.
// The function returns the pointer to the next char in 'b' beyond the
// replaced character to allow for further parsing.
// If nothing is found then NULL is returned.
// If at the end of 'b' then the returning pointer will be the '\0' 
// found at the end of 'b'.
char * sutil_strtok(char * b, const char * m) {
    char * n = NULL;
    if (b && m && *b != '\0') {
        int i = 0;
        int j = 0;
        int fmatch = 1; // 1 := find 1st token, 0 find next non-matching token
        int bL = sutil_strlen(b);
        int mL = sutil_strlen(m);
        //printf("{sutil_strtok} bL[%d] mL[%d]\n", bL, mL);
        do {
            if (b[i] == '\0') {
                n = b;
                //printf("{sutil_strtok} 'b' is empty, at EOL\n");
                break;
            }            
            for ( j = 0 ; j < mL ; j++ ) {
                if (fmatch && b[i] == m[j]) {
                    b[i] = '\0';
                    fmatch = 0;
                    //printf("{sutil_strtok} 1st token match, i[%d] j[%d], fmatch CLEARED\n", i, j);
                    break;
                } else if (!fmatch && b[i] == m[j]) {
                    b[i] = '\0';
                    //printf("{sutil_strtok} next token match, i[%d] j[%d]\n", i, j);
                    break;
                }
            }
            if (!fmatch && j == mL) {
                // no token match when trying to find multiple 
                // occurences of them, dropped out of the bottom
                // of the for-loop. setup the token match and set 
                // string remainder.
                //printf("{sutil_strtok} no remaining token match, i[%d] j[%d] - n is SET\n", i, j);
                n = b+i; // already at the next char passed the token(s)
            }
            i ++;
        } while (n == NULL && i < bL);
        if (i == bL && !fmatch) {
            // ran off end of string while trying to skip over 
            // multiple token occurances, this was doing a match
            // so finish it properly.
            n = b+i; // set to the EOL character!
        }
    }
    return n;
}

int sutil_strcmp(const char * a, const char * b) {
    int32_t sa = (a) ? sutil_strlen(a) : 0;
    int32_t sb = (b) ? sutil_strlen(b) : 0;
    if (sa != sb)
        return (int)(sa - sb);
    for (sa = 0 ; sa < sb ; sa++) {
        if (a[sa] != b[sa])
            return (int)a[sa] - (int)b[sa];
    }
    return 0;
}

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

uint8_t sutil_asciinumber(char * at, int32_t ival) {
    uint8_t sp = 0;
    uint8_t mval = 0;
    uint8_t do_insert_zeros = 0; // flag once the highest multiplier found.
    int32_t mult = 1000000000; // max. range (=ve) is: 0x7fffffff := 2,147,483,647
    if (ival == 0x80000000) {
        // max. negative cannot be factored, just return a hard-coded result.
        sp = (uint8_t)sutil_strcpy(at,"-2147483648");
        return sp;
    } else if (ival < 0) {
        at[sp++] = '-';
        ival = ival * -1;
    } 
    if (ival == 0) {
        at[sp++] = '0';
    } else {
        while (mult > 0) {
            if (ival >= mult) {
                mval = ival / mult;
                ival = ival - (mult * mval);
                at[sp++] = '0' + mval;
                do_insert_zeros = 1; // set, if not already done.
            } else if (do_insert_zeros) {
                at[sp++] = '0';
            }
            mult = mult / 10;
        }
    }
    return sp;
}

