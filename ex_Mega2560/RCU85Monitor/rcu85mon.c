/*********************************************************************
 * rcu85mon.c
 * Main loop for the RCU85 CPU-Board "Dacosta-bot" Hardware Monitor
 * Version 1.0
 * ---
 * The hardware monitor is to provide operations as a controller for the
 * LED display & Keyboard (key switches). In addition, it supports a 
 * serial monitor that can be used to read and write RCU85 memory for
 * monitoring operations and editing embedded assembler code.
 * 
 * ----- Makefile ----------------------------------------------------
 * SET FOLLOWING DEFINES:
 * P_MAX_VERBCOUNT      10
 * P_MAX_VERBLEN        8
 * P_MAX_CMDLEN         80
 * TEMP_BUF_LEN         80
 * P_OK_ON_SUCCESS      (just define it)
 * MAX_GPIO_RSVD        10 (!) ADJUST WHEN RCU85 MEMORY I/F IS ADDED
 * 
 **********************************************************************/

#include <avrlib/avrlib.h>
#include <avrlib/gpio_api.h> 
#include <avrlib/dblink.h>
#include <avrlib/stringutils.h>
#include <avrlib/driver.h>
#include <avrlib/cmdparser.h>
#include <avrlib/libtime.h>
#include "kybd_led_io.h"
#include "rcu85cmds.h"
#include "rcu85mem.h"

// (TODO) will need to be externally accessed...
kybd_t mon_info; // data from the serial monitor

#if 0
static uint8_t tcount[3] = {0,0,0};
static void test_display(kybd_t * kbd_info) {
    int rc;
    kbd_info->ds.dat.data = tcount[0];
    kbd_info->ds.dat.addr_lo = tcount[1];
    kbd_info->ds.dat.addr_hi = tcount[2];
    
    rc = disp_update(kbd_info);
    if (rc) {
        blink_error(6);
    }
    
    if (++tcount[0] == 0)
        if (++tcount[1] == 0)
            ++tcount[2];
    
}
#endif

int main (void) {
    kybd_t kbd_info; // data from the keyboard
	int  fhnd;
    int rc;
	
	// Setup the System drivers and the driver stack
	System_DriverStartup();
    System_driverInit();
	blink_init();           /* also initializes gpiolib... */
	rc = kybdio_sysinit();       /* setup keyboard & LED display */
    if (rc != KD_SUCCESS) {
        if (rc == KD_ERR_KYBD) {
            blink_error(4); /* keyboard - init */
        } else {
            blink_error(5); /* LED Display - init */
        }
    }
    // Setup CPU Bus interface
    rc = rcmem_init();
    if (rc != RCM_SUCCESS) {
        blink_error(6);
    }
    
    // Pre-load monitor data
    {
        mon_info.ds.dat.data = 0;
        mon_info.ds.dat.addr_hi = 0;
        mon_info.ds.dat.addr_lo = 0;
        mon_info.ds.dat._unused = 0;
    }
    
    // Open Command Perser's interface
    fhnd = startParser("/dev/uart/1,19200,8,N,1", 0);
	if (fhnd != 0)
		blink_error(2);

	tm_delay_ms(10);

	/* Monitor Title message --> serial terminal (if attached) */
    rcmd_sendWelcome();

#if 0
    if (disp_isInitializaed())
        pSendString("LED Display setup - OK\r\n");
    if (kybd_isInitializaed())
        pSendString("Key switch panel setup - OK\r\n");
#endif

    /* === MAIN LOOP =================================================*/

    while (1) {
        tm_delay_ms(20);
        if ( pollParser() != 0 ) {
            blink_error(3);
        }

        if ((rc = kybd_scan(&kbd_info)) != KD_SUCCESS)
            blink_error(5); /* Keyboard - Scan error */

        if ( rcmd_readMonitorState() == MSTATE_KYBD) {
            if ((rc = disp_update(&kbd_info)) != KD_SUCCESS)
                blink_error(7); /* LED Display - update error */
        } else {
            if ((rc = disp_update(&mon_info)) != KD_SUCCESS)
                blink_error(7); /* LED Display - update error */
        }
    }
    return(0);
}

