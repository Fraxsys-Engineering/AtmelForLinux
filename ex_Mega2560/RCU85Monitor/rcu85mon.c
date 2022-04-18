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

const char * sHelp  = "\
System help menu ----------------------------------------------------\r\n\
  help      (h)                      display command help (this menu)\r\n\
  read      (r)  [addr len]          read memory. addr, len are hex. \r\n\
  write     (w)  [addr d0 [d1 .. d7]] write 1 - 8 bytes, addr, data are hex.\r\n\
";

#define RX_STRN_MAX 128

const char * starts = "RCU85 Monitor - Ver 1.0\r\n";

// Print out one line of 8 bytes, and a starting address. All in hex.
// Less than 8 bytes to print will place blanks '--' for remainder.
static void prline8( uint16_t addr, const uint8_t * bytes, int len ) {
    pSendString("[");
    pSendHexShort(addr);
    pSendString("]  ");
    int i = 0;
    while (len) {
        pSendHexByte(bytes[i]);
        pSendString(" ");
        i++;
        len --;
    }
    while (i < 8) {
        pSendString("-- ");
        i ++;
    }
    pSendString("\r\n");
}

// Print out memory in a series of 8 octet lines.
// TODO - UPDATE to reflect reading of RCU85 memory.
static void prmem( uint16_t addr, int len ) {
    uint8_t bytbyt[8];
    while (len) {
        int cplen = (len < 8) ? len : 8;
        sutil_memcpy((void *)bytbyt, (void *)addr, cplen);
        prline8(addr, bytbyt, cplen);
        len -= cplen;
        addr += cplen;
    }
}

// [[COMMAND]] 'help' | 'h' - nargs: 0
// Display help screen
static int cmd_help(int vc, const char * verbs[]) {
    pSendString(sHelp);
	return CMD_SUCCESS;
}

static int cmd_read(int vc, const char * verbs[]) {
#ifdef TDD_PRINTF    
	printf("(cmd_read) vc:%d\n", vc);
#endif
    pSendString("cmd_read\r\n");
	return CMD_SUCCESS;
}

static int cmd_write(int vc, const char * verbs[]) {
#ifdef TDD_PRINTF    
	printf("(cmd_write) vc:%d\n", vc);
#endif
    pSendString("cmd_write\r\n");
	return CMD_SUCCESS;
}

cmdobj pCommandList[4] = {
	{ "help", "h", 0, 0, cmd_help},
	{ "read", "r", 2, 2, cmd_read},
    { "write","w", 2, 9, cmd_write},
	{ NULL, NULL,  0, 0, NULL}
};

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
    kybd_t kbd_info;
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
    
    // Open Command Perser's interface
    fhnd = startParser("/dev/uart/1,9600,8,N,1", 0);
	if (fhnd != 0)
		blink_error(2);

	tm_delay_ms(10);

	/* Monitor Title message --> serial terminal (if attached) */
    pSendString(starts);

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
#if 1        
        if ((rc = kybd_scan(&kbd_info)) != KD_SUCCESS)
            blink_error(5); /* Keyboard - Scan error */
        if ((rc = disp_update(&kbd_info)) != KD_SUCCESS)
            blink_error(6); /* LED Display - update error */
#endif
#if 0
        test_display(&kbd_info);
#endif
    }

    return(0);
}

