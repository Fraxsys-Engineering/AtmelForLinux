/**********************************************************************
 * dblink.h
 *
 * Debugging support library
 *
 * REQUIRES:
 *
 *********************************************************************/

#ifndef _DBLINK_H_
#define _DBLINK_H_

#define LED_PIN_DDR DDRB
#define LED_PIN_BIT DDB7
#define LED_PIN_PRT PORTB 

void blink_init(void);
void blink_once(int count);
void blink_error(int count); // acts as an error trap

#endif /* _DBLINK_H_ */
