/**********************************************************************
 * dblink.c
 * Debugging support library
 *********************************************************************/

#include "dblink.h"
#include "libtime.h"

void blink_init(void) {
	LED_PIN_DDR |= (1 << LED_PIN_BIT);
	LED_PIN_PRT &= ~(1 << LED_PIN_BIT);
} 

void blink_once(int count) {
    int i;
	for (i = 0 ; i < count ; ++i) {
		LED_PIN_PRT |= (1<< LED_PIN_BIT);
		tm_delay_ms(100);
		LED_PIN_PRT &= ~(1 << LED_PIN_BIT);
		tm_delay_ms(100);
	}
}

// acts as an error trap
void blink_error(int count) {
    LED_PIN_DDR |= (1 << LED_PIN_BIT);
    while (1) {
		blink_once(count);
        tm_delay_ms(1000);
    }
}

