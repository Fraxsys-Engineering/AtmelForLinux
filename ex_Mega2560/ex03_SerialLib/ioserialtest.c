/****************************************************************************
 * Excercise 03 - First library: USART Serial driver
 * ---
 * ioserialtest.c
 * Test the new USART Serial driver ("SER1" @ 9600 8N1 Async.)
 * Version 1.0
 * ---
 * SHORT - Support and Testing code for the new Serial driver 
 *         avrlib/serialdriver.*
 * ---
 * TODO PHASES
 ***************************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>  // uint8_t etc.
#include <string.h>  // string ops.
#include <avr/interrupt.h> // TESTING ONLY - REMOVE WHEN DONE
/* Our Libraries */
#include <avrlib/dblink.h>
#include <avrlib/serialdriver.h>

const char * starts = "Program Start\r\n";
const char * prompt = "\r\nEnter a String with carriage-return > ";
const char * ends   = "Program End\r\n";

#if 0
#define HI_NIB 1
#define LO_NIB 0
static char asciihex_nibble( uint8_t n, uint8_t isHiNibble ){
	static char nbchar[] = "0123456789ABCDEF";
	if (isHiNibble) {
		n = (n >> 4) & 0x0f;
	} else {
		n = n & 0x0f;
	}
	return nbchar[n];
}

#define AH_PREFIX 1
#define NO_PREFIX 0
// place ASCII-HEX string value of 'hval' at pointer 'at' and 'at'+1,
// or at 'at'+2,3 and prefix string 'at' with "0x"
// return # bytes advanced into 'at'
static uint8_t asciihex( char * at, uint8_t hval, uint8_t prefix ) {
	uint8_t ptr = 0;
	if (prefix) {
		at[ptr++] = '0';
		at[ptr++] = 'x';
	}
	at[ptr++] = asciihex_nibble( hval, HI_NIB );
	at[ptr++] = asciihex_nibble( hval, LO_NIB );
	return ptr;
}
#endif

#define RX_STRN_MAX 128

int main () {
	char rx_string[RX_STRN_MAX];
#if 0
	char db_string[64];
	int  dbptr = 0;
#endif
	int  endptr = 0;
	int  readc;
	
	blink_init();
	
	// Test Serial Operations via avrlib/serialdriver
	if (serial_open(9600, SF_8, SS_1, SP_NONE) != 0) {
		blink_error(2);
	}

	_delay_ms(10);
	if (serial_putstrn(starts) != strlen(starts)) {
		blink_error(4);
	}

    while (1) {
		blink_once(1);

		if (serial_putstrn(prompt) != strlen(prompt)) {
			blink_error(4);
		}
		
		// wait for a return that ends in a carriage return (cr-lf)
		rx_string[0] = '\0';
		endptr = 0;
		while (1) {
			_delay_ms(100);
			if (serial_rd_peek() > 0) {
				readc = serial_gets(&(rx_string[endptr]), RX_STRN_MAX - endptr);
				if (readc < 0) {
					blink_error(6);
				}
				// remote echo
				serial_puts(&(rx_string[endptr]), readc);
#if 0
				dbptr = (int)asciihex(db_string, (uint8_t)readc, NO_PREFIX );
				db_string[dbptr++] = '\r';
				db_string[dbptr++] = '\n';
				db_string[dbptr++] = '\0';
				serial_putstrn(db_string);
#endif
				endptr += readc;
				if (endptr && (rx_string[endptr-1] == '\r' || rx_string[endptr-1] == '\n')) {
					break;
				}
			}
		}
		// Reflect the message back
		//rx_string[endptr] = '\0';
		//serial_putstrn(rx_string);
		serial_putstrn("\r\nOK\r\n");
    }
    
    serial_putstrn(ends);
    _delay_ms(100);
    
}

