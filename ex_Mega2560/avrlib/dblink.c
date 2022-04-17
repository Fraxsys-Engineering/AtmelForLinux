/**********************************************************************
 * dblink.c
 * Debugging support library
 *********************************************************************/

#include <avrlib/gpio_api.h>
#include <avrlib/libtime.h>
#include <avr/io.h>
#include "dblink.h"

#define LED1_PORT   PM_PORT_B
#define LED1_PIN    PM_PIN_7

static int led1_hndl = 0;

void blink_init(void) {
    if ( !pm_isInitialized() )
        pm_init();
        
    led1_hndl = pm_register_pin(LED1_PORT, LED1_PIN, PINMODE_OUTPUT_LO);
    pm_out(led1_hndl, 0);
} 

void blink_once(int count) {
    int i;
	for (i = 0 ; i < count ; ++i) {
		//pm_tog(led1_hndl);
        pm_out(led1_hndl, 1);
		tm_delay_ms(100);
		//pm_tog(led1_hndl);
        pm_out(led1_hndl, 0);
		tm_delay_ms(100);
	}
}

// acts as an error trap
void blink_error(int count) {
    while (1) {
		blink_once(count);
        tm_delay_ms(1000);
    }
}

