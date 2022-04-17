/****************************************************************************
 * Exercise 04 - Expending Serial Support - Mounting a Command Parser
 * ---
 * sercmdparser.c
 * Mount a Command parser on Serial device /dev/uart/1 @ 9600 8N1 Async.
 * Version 1.0
 * ---
 * SHORT - Support and Testing code for the Command Parser framework.
 * ---
 * CHANGELOG
 * Ver 1.0	
 * 
 * ----- Makefile -----------------------------------------------------
 * SET FOLLOWING DEFINES:
 * P_MAX_VERBCOUNT      10
 * P_MAX_VERBLEN        8
 * P_MAX_CMDLEN         80
 * TEMP_BUF_LEN         80
 * P_OK_ON_SUCCESS      (just define it)
 * 
 ***************************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>  // uint8_t etc.
#include <string.h>  // string ops.
/* Our Libraries */
#include <avrlib/stringutils.h>
#include <avrlib/driver.h>
#include <avrlib/cmdparser.h>
#include <avrlib/gpio_api.h>
#include <avrlib/dblink.h>

const char * sHelp  = "\
System help menu ----------------------------------------------------\r\n\
  help      (h)                      display command help (this menu)\r\n\
  read      (r)  [addr len]          read memory. addr, len are hex. \r\n\
  write     (w)  [addr d0 [d1 .. d7]] write 1 - 8 bytes, addr, data are hex.\r\n\
";

#define RX_STRN_MAX 128

const char * starts = "Serial Command Parser - Ver 1.0\r\n";

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


int main () {
	int  fhnd;
	
	// Setup the System drivers and the driver stack
	System_DriverStartup();
    System_driverInit();
    //pm_init(); <-- dblink will now handle this for you.
	blink_init();
	
    // Open Command Perser's interface
    fhnd = startParser("/dev/uart/1,9600,8,N,1", 0);
	if (fhnd != 0)
		blink_error(2);

	_delay_ms(10);

	// Send the test start confirmation string...
    pSendString(starts);

#if 0
    {
        char nbuf[8];
        uint8_t byt, len;
        uint16_t addr;
        // debug gpio_api using dblink
        // dblink uses PortB, Pin 7. It should be set to output.
        // [1] Check DDRB, PORTB for state
        // [2] Report the address of DDRB
        pSendString("DDRB[");
        byt = DDRB;
        len = sutil_asciihex_byte(nbuf, byt, SU_NO_PREFIX, SU_LOWERCASE);
        nbuf[len] = '\0';
        pSendString(nbuf);
        pSendString("] PORTB[");
        byt = PORTB;
        len = sutil_asciihex_byte(nbuf, byt, SU_NO_PREFIX, SU_LOWERCASE);
        nbuf[len] = '\0';
        pSendString(nbuf);
        pSendString("]\r\n");
        pSendString("DDRB ADDR[");
        addr = (uint16_t)&(DDRB);
        len = sutil_asciihex_word(nbuf, addr, SU_NO_PREFIX, SU_LOWERCASE);
        nbuf[len] = '\0';
        pSendString(nbuf);
        pSendString("]\r\n");
    }
#endif


    while (1) {
        _delay_ms(20);

        if ( pollParser() != 0 ) {
            blink_error(3);
        }
    }

    return(0);
}

