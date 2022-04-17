/**********************************************************************
 * gpiotest.c
 * Test new GPIO API using the onboard LED's
 * 
 * This test reworks dblink to use the new gpio API and thus reserve
 * the GPIO pin tied to this LED.
 * It will define common types specific to avrlib as well, eg. memory 
 * pointer types.
 * 
 * Version 1.0
 *
 **********************************************************************/

#include <avrlib/dblink.h>
#include <avrlib/libtime.h>

int main (void) {
    // initialize the blinker API
    blink_init();

    // blink forever
    while (1) {
        blink_once(4);
        tm_delay_ms(1500);
    }
}

