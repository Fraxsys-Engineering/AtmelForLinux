/****************************************************************************
 * kybd_led_io.h
 * Keyboard Scan and 7-Segment LED Dispay Updater
 * Version 1.1
 * ---
 * SHORT - Design as the glue logic for the RCU-85 Programming keyboard. 
 *         This will eventually also connect into the 8085A Memory bus and
 *         act as a monitor to program and read RAM Data.
 * 
 * Ver 1.1      Ported to an API to be used by rcu85_monitor
 * 
 * ---
 * TODO PHASES
 * [B] Add ability to HOLD 8085 CPU and directly access the memory space.
 *     ref. Atmel Mega2560 EMI interface or GPIO if EMI is too fast.
 * [C] Serial interface and Monitor CLI. Support for file downloading
 *     X,Y,Z-Modem? into a local RAM buffer then off-loading into RCU-85
 *     Memory.
 ***************************************************************************/

#ifndef _KYBD_LED_IO_H_
#define _KYBD_LED_IO_H_

#include <avrlib/avrlib.h>

/* Some functions return a success/fail code -------------------------*/
#define KD_SUCCESS  0
#define KD_ERROR    (-1)
#define KD_ERR_KYBD (-2)
#define KD_ERR_DISP (-3)

/* To accomodate 4 byte int bit-mask, extra byte will be used in 
 * future update. 
 */
#define KYBS_STRUCTBUS_SZ   4

/* Keyboard Data Structure - each scan will update an instance of 
 * this struct. Caller must create and hold struct instance.
 */
typedef struct keybd_type {
    union ds_type {
        // raw bytes. right-most shifter gets loaded into 
        // byt[0] then next shifter gets into the next byte.
        // bytes shift RIGHT while b7 is updated (ITER8: in->b7; shift-right)
        uint8_t byt[KYBS_STRUCTBUS_SZ];
        struct dat_type {
            uint8_t data;
            uint8_t addr_lo;
            uint8_t addr_hi;
            uint8_t _unused;
        } dat;
        struct bit_type {
            int d7  : 1;
            int d6  : 1;
            int d5  : 1;
            int d4  : 1;
            int d3  : 1;
            int d2  : 1;
            int d1  : 1;
            int d0  : 1;
            int a7  : 1;
            int a6  : 1;
            int a5  : 1;
            int a4  : 1;
            int a3  : 1;
            int a2  : 1;
            int a1  : 1;
            int a0  : 1;
            int a15 : 1;
            int a14 : 1;
            int a13 : 1;
            int a12 : 1;
            int a11 : 1;
            int a10 : 1;
            int a9  : 1;
            int a8  : 1;
            int _uu : 8;
        } bit;
    } ds;
} kybd_t;

/* Externally defined memory for holding monitor settings - to be 
 * updated in the LED display 
 */
extern kybd_t mon_info;

/* Initialize operations for Keyboard scanning and LED refreshing ----
 * RETURNS: 
 *      KD_SUCCESS | 
 *      KD_ERR_KYBD (Keyboard init) | 
 *      KD_ERR_DISP (Display init)
 */
int kybdio_sysinit(void);
uint8_t disp_isInitializaed(void);
uint8_t kybd_isInitializaed(void);

/* Scan keyboard ----------------------------------------------------- 
 * Keyboard states are updated in 'data'
 * RETURNS: 
 *      KD_SUCCESS | 
 *      KD_ERR_KYBD (Keyboard read)
 */
int kybd_scan(kybd_t * data);

/* Use given data in 'disp' to update the LED display ----------------
 * RETURNS: 
 *      KD_SUCCESS | 
 *      KD_ERR_DISP (Display init)
 */
int disp_update(kybd_t * disp);

#endif /* _KYBD_LED_IO_H_ */
