/**********************************************************************
 * Exercise. 03b - GPIO Library
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
 * ADDITIONAL: 
 *  [1] Insert a LED with 1K current limiter into PB6 (anode), cathode 
 *      to GND. PB6 = (12)
 *  [2] Insert a switch to GND in PB5 (11).
 * PB6 LED will flash when SW PB5 is pressed.
 **********************************************************************/

#include <avrlib/gpio_api.h>
#include <avrlib/dblink.h>
#include <avrlib/libtime.h>

int main (void) {
    int hndl_pb6_led = 0;
    int hndl_pb5_sw = 0;
    uint16_t tmr = 0;
    uint8_t  db_sw5 = 0;
    
    // initialize the blinker API
    blink_init();

    // Setup test LED
    if ((hndl_pb6_led = pm_register_pin(PM_PORT_B, PM_PIN_6, PINMODE_OUTPUT_LO)) < 1)
        blink_error(1);
        
    // Setup test switch
    if ((hndl_pb5_sw = pm_register_pin(PM_PORT_B, PM_PIN_5, PINMODE_INPUT_PU)) < 1)
        blink_error(2);
    
    tm_delay_ms(20);
    
    while (1) {
        blink_once(4);
        while (tmr < 2000) {
            tm_delay_ms(20);
            tmr += 20;
            // debounce sw5 - active for 100 ms
            if (pm_in(hndl_pb5_sw) == 0) {
                if (db_sw5 < 6)
                    ++db_sw5;
                if (db_sw5 > 4) {
                    db_sw5 = 6; // if it hist 5 then bump to 6 to disable count
                    pm_out(hndl_pb6_led,1); // set the led
                }
            } else {
                if (db_sw5 == 6) {
                    // reach a stable on, turn off led.
                    pm_out(hndl_pb6_led,0);
                }
                db_sw5 = 0;
            }
        } // inner loop, debounce sw5 and update led
        tmr = 0;
    } // outer loop, blink main led pattern.
}

