#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN_DDR DDRB
#define LED_PIN_BIT DDB7
#define LED_PIN_PRT PORTB 

int main (void) {
    // set LED GPIO as OUTPUT
    LED_PIN_DDR |= (1 << LED_PIN_BIT);

    // blink forever
    while (1) {
        // Set LED
        LED_PIN_PRT |= (1<< LED_PIN_BIT);

        // wait
        _delay_ms(100);

        // Clr LED
        LED_PIN_PRT &= ~(1 << LED_PIN_BIT);
        
        // wait
        _delay_ms(900);
    }
}
