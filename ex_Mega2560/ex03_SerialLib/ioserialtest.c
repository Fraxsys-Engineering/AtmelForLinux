/****************************************************************************
 * Exercise 03 - First library: USART Serial driver
 * ---
 * ioserialtest.c
 * Test the new USART Serial driver ("SER1" @ 9600 8N1 Async.)
 * Version 2.0
 * ---
 * SHORT - Support and Testing code for the new Serial driver 
 * ---
 * CHANGELOG
 * Ver 1.0	Testing simple serial driver: avrlib/serialdriver.*
 * Ver 2.0  Testing full driver stack & /dev/uart/1 (minor device 1)
 ***************************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>  // uint8_t etc.
#include <string.h>  // string ops.
/* Our Libraries */
#include <avrlib/dblink.h>
#include <avrlib/chardev.h>   // user code access to the character stream devices
#include <avrlib/driver.h>    // main (system) code must be able to call these methods

const char * starts = "Program Start\r\n";
const char * prompt = "\r\nEnter a String with carriage-return > ";
const char * acks   = "\r\nOK\r\n";
const char * ends   = "Program End\r\n";

#define RX_STRN_MAX 128

int main () {
	char rx_string[RX_STRN_MAX];
	int  fhnd;
	int  endptr = 0;
	int  readc;
	
	// Setup the System drivers and the driver stack
	System_DriverStartup();
    System_driverInit();
	blink_init();
	
	// Open uart minor port 1 (0 is reserved!)
	fhnd = cd_open("/dev/uart/1,9600,8,N,1", 0);
	if (fhnd <= 0)
		blink_error(2);

	_delay_ms(10);
	// Send the test start confirmation string...
	if (cd_write(fhnd, starts, strlen(starts)) != strlen(starts)) {
		blink_error(4);
	}

    while (1) {
		blink_once(1);

		// Send the Test command request string (repeats)
		if (cd_write(fhnd, prompt, strlen(prompt)) != strlen(prompt)) {
			blink_error(4);
		}

		// USER SHOULD BE SENDING ASCII CHARS BACK TO THE DUT
		
		// wait for a return that ends in a carriage return (cr-lf)
		rx_string[0] = '\0';
		endptr = 0;
		while (1) {
			_delay_ms(100);
			
			readc = cd_read(fhnd, &(rx_string[endptr]), RX_STRN_MAX - endptr);
			if (readc < 0) {
				blink_error(6);
			}
			if (readc > 0) {
				// remote echo
				cd_write(fhnd, &(rx_string[endptr]), readc);
			
				// update the rx line buffer and test for a string end condition
				endptr += readc;
				if (endptr && (rx_string[endptr-1] == '\r' || rx_string[endptr-1] == '\n')) {
					break; // received a newline/carriage-return
				}
			}
		}
		// ACK
		cd_write(fhnd, acks, strlen(acks));
    }

	cd_write(fhnd, ends, strlen(ends));
    _delay_ms(100);
    
    cd_close(fhnd);  
    fhnd = 0;
    return(0);
}

