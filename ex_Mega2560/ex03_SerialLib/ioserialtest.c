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
/* Our Libraries */
#include <avrlib/serialdriver.h>

const char * prompt = "Enter a String with carriage-return > ";

#define LED_PIN_DDR DDRB
#define LED_PIN_BIT DDB7
#define LED_PIN_PRT PORTB 
static void blink_init(void) {
	LED_PIN_DDR |= (1 << LED_PIN_BIT);
	LED_PIN_PRT &= ~(1 << LED_PIN_BIT);
} 

static void blink_once(int count) {
    int i;
	for (i = 0 ; i < count ; ++i) {
		LED_PIN_PRT |= (1<< LED_PIN_BIT);
		_delay_ms(100);
		LED_PIN_PRT &= ~(1 << LED_PIN_BIT);
		_delay_ms(100);
	}
}

// acts as an error trap
static void blink_error(int count) {
    LED_PIN_DDR |= (1 << LED_PIN_BIT);
    while (1) {
		blink_once(count);
        _delay_ms(1000);
    }
}

#define RX_STRN_MAX 128
int main () {
	char rx_string[RX_STRN_MAX];
	int headptr = 0;
	int endptr = 0;
	int readc,writec;
	
	blink_init();
	
	// Test Serial Operations via avrlib/serialdriver
	if (serial_open(9600, SF_8, SS_1, SP_NONE) != 0) {
		blink_error(2);
	}

    while (1) {
		blink_once(1);
		_delay_ms(100);
		if (serial_putstrn(prompt) != strlen(prompt)) {
			blink_error(4);
		}
		// wait for a return that ends in a carriage return (cr-lf)
		rx_string[0] = '\0';
		headptr = 0;
		endptr = 0;
		while (1) {
			_delay_ms(10);
			if (serial_peek() > 0) {
				readc = serial_gets(&rx_string[endptr], RX_STRN_MAX - endptr);
				if (readc < 0) {
					blink_error(6);
				}
				endptr += readc;
				if (endptr >= (RX_STRN_MAX-1) || rx_string[endptr-1] == '\r' || rx_string[endptr-1] == '\n') {
					break;
				}
			}
		}
		// Reflect the message back
		while (headptr < endptr) {
			if ((writec = serial_puts(&rx_string[headptr], (endptr - headptr))) < 0) {
				blink_error(8);
			}
			headptr += writec;
			_delay_ms(40);
		}
    }
}

