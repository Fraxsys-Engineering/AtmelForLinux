/**********************************************************************
 * dblink.h
 *
 * Debugging support library - Ver 1.1
 *
 * REQUIRES: apio_api, avrlib.h
 *
 *********************************************************************/

#ifndef _DBLINK_H_
#define _DBLINK_H_

void blink_init(void);
void blink_once(int count);
void blink_error(int count); // acts as an error trap

#endif /* _DBLINK_H_ */
