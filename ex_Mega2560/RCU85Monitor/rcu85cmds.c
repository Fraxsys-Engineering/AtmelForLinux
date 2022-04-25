/*********************************************************************
 * rcu85cmds.c
 * 
 * Version 1.0
 * ---
 * Commands for the RCU85 Monitor
 * 
 * Command List:
 *  help    (h)         display help menu
 *  mode    (m)         set operating mode
 *  read    (r)         read memory
 *  write   (w)         write 1..8 bytes to given address
 *  addr    (a)         set start address
 *  bwrt    (b)         send a "bulk" write, for loading programs
 * 
 **********************************************************************/

#include "rcu85cmds.h"
#include "rcu85mem.h"
#include "kybd_led_io.h"
#include <avrlib/stringutils.h>
#include <avrlib/cmdparser.h>
#ifdef TDD_PRINTF 
 #include <stdio.h>
#endif

const char * starts = "RCU85 Monitor - Ver 1.0\r\n";

const char * sHelp  = "\
System help menu ----------------------------------------------------\r\n\
  help      (h)                display command help (this menu)\r\n\
  mode      (m)  [term | kybd] Set/Read display driver: \r\n\
              term(inal) - display shows serial terminal input\r\n\
              kybd (keyboard) - display shows local keyboard input\r\n\
              (no argument) - read the current state\r\n\
  iom       (im) [io | mem]    Set/get target address space to be either I/O\r\n\
                               'io' or RAM memory 'mem'. Deafults to mem.\r\n\
                               If no args then returns the current setting.\r\n\
  read      (r)  [addr len]    read mem / IO. addr, len are hex. max 64 bytes.\r\n\
  write     (w)  [addr d0 [d1 .. d7]]\r\n\
                               write 1 - 8 bytes, addr, data are hex.\r\n\
  addr      (a) [addr]         set address pointer (for bulk writes)\r\n\
  bwrt      (b) [data...] EOF  Send bulk data, starting from set addr.\r\n\
                               finish the transfer by sending ETX (0x03)\r\n\
  halt      (hold)             Hold the RCU85 in a Halted state until \r\n\
                               released using run (go)\r\n\
  run       (go) [reset]       Release the RCU85, when being manually held.\r\n\
                               If optional 'reset' is added then CPU is reset.\r\n\
  NOTE: halt,run not required as a write will perform a halt,release. If \r\n\
   the CPU is to be held during multiple read,write ops then call 'halt'\r\n\
   first, perform reads,writes then manually release the CPU using 'run'.\r\n\
  NOTE: Hexadecimal values DO NOT need a preceeding '0x'. Just type the\r\n\
   value using 0-9,a-f,A-F characters.\r\n\
  NOTE: Bulk write (bwrt) data is 2 character ASCII-HEX, no leading '0x'.\r\n\
   data can be separated by white-space as an option. End of data stream is \r\n\
   signalled by a (0x)03 byte.\r\n\
";

void rcmd_sendWelcome(void) {
    pSendString(starts);
}

void rcmd_sendHelpMenu(void) {
    pSendString(sHelp);
}


/* === COMMAND SUPPORT ===============================================*/

typedef enum memtype_type {
    MTYP_RAM = 0,
    MTYP_IO = 1,
    /* ... */
    MTYP_COUNT
} memtype_t;

static memtype_t memtype = MTYP_RAM;

const char * s_memtype[MTYP_COUNT] = { "mem", "I/O" };

const char * s_monstate[MSTATE_COUNT] = { "keyboard", "monitor" };

// Current Monitor Operating State
static mon_state_t mon_state = MSTATE_KYBD;

mon_state_t rcmd_readMonitorState(void) {
    return mon_state;
}

// General Purpose READ/WRITE buffer, shared by multiple commands.
#ifndef MAX_MEMBUF_LEN
#define MAX_MEMBUF_LEN  64
#endif
static uint16_t g_memptr = 0;
static uint8_t  g_membuf[MAX_MEMBUF_LEN];
// Local Address cache - This is updated to support raw write data
// sequences, 'bwrt'. Not used for 'read' 'write' command ops.
static uint16_t bwAddr = 0;

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
static void prmem( uint16_t addr, const uint8_t * buf, int len ) {
    uint8_t bytbyt[8];
    while (len) {
        int cplen = (len < 8) ? len : 8;
        sutil_memcpy((void *)bytbyt, (void *)buf, cplen);
        prline8(addr, bytbyt, cplen);
        len -= cplen;
        addr += cplen;
        buf += cplen;
    }
}


/* === COMMAND LIST ==================================================*/

// [[COMMAND]] 'help' | 'h' - nargs: 0
// Display help screen
static int cmd_help(int vc, const char * verbs[]) {
    pSendString(sHelp);
	return CMD_SUCCESS;
}

// [[COMMAND]] 'mode' | 'm' - nargs: 0,1
// Read or set Operating Mode
static int cmd_mode(int vc, const char * verbs[]) {
    int rc = CMD_SUCCESS;
    if (vc) {
        if (sutil_strcmp(verbs[0], "kybd") == 0) {
            mon_state = MSTATE_KYBD;
        } else if (sutil_strcmp(verbs[0], "term") == 0) {
            mon_state = MSTATE_TERM;
        } else {
            rc = CMD_ERROR_SYNTAX;
        }
    }
    if (rc == CMD_SUCCESS) {
        pSendString("Monitor mode: ");
        pSendString(s_monstate[mon_state]);
        pSendString("\r\n");
    }
	return rc;
}

static int cmd_iom(int vc, const char * verbs[]) {
    int rc = CMD_SUCCESS;
    if (vc) {
        if (sutil_strcmp(verbs[0], "io") == 0) {
            memtype = MTYP_IO;
        } else if (sutil_strcmp(verbs[0], "mem") == 0) {
            memtype = MTYP_RAM;
        } else {
            rc = CMD_ERROR_SYNTAX;
        }
    }
    if (rc == CMD_SUCCESS) {
        pSendString("addr map: ");
        pSendString(s_memtype[memtype]);
        pSendString("\r\n");
    }
    return rc;
}

static int cmd_read(int vc, const char * verbs[]) {
    // <addr> <count>
    int rc = CMD_SUCCESS;
    int rcount = 0;
    uint16_t addr;
    uint8_t  is_held = rcmem_isHeld();
    g_memptr = 0;
    if (vc == 2) {
        addr = sutil_strtohex(verbs[0]);
        g_memptr = sutil_strtohex(verbs[1]);
        if (is_held) {
            pSendString("READ: pre-hold on\r\n");
        }
        // If not already HALTed, do a localized HALT..RUN around the Write operation
        if (is_held || rcmem_hold() == RCM_SUCCESS ) {
            if ( !rcmem_isHeld() ) {
                pSendString("READ ERROR: hold request failed\r\n");
            } else if ( (rcount = rcmem_read(addr, g_membuf, g_memptr, (uint8_t)memtype)) != g_memptr) {
                if (rcount >= 0) {
                    char numbuf[16];
                    int  numptr;
                    pSendString("READ ERROR (count mis-match) req[");
                    numptr = sutil_asciinumber(numbuf, g_memptr); numbuf[numptr] = '\0';
                    pSendString(numbuf);
                    pSendString("] rcvd[");
                    numptr = sutil_asciinumber(numbuf, rcount); numbuf[numptr] = '\0';
                    pSendString(numbuf);
                    pSendString("]\r\n");
                } else {
                    pSendString("READ ERROR (Operation Error)\r\n");
                }
            } else if (is_held || rcmem_release(NO_CPU_RESET) == RCM_SUCCESS) {
                prmem(addr, g_membuf, g_memptr);
            } else {
                pSendString("RCU85 release operation failed.\r\n");
            }
        } else {
            pSendString("RCU85 hold operation failed.\r\n");
        }
    } else {
        rc = CMD_ERROR_SYNTAX;
    }
	return rc;
}

static int cmd_write(int vc, const char * verbs[]) {
    // <addr> <byt> [<byt> ...] up to 8 bytes of data
    int rc = CMD_SUCCESS;
    uint16_t addr;
    uint8_t  vcp = 0;
    uint8_t  is_held = rcmem_isHeld();
    g_memptr = 0;
    if (vc >= 2) {
        addr = sutil_strtohex(verbs[vcp++]);
        while (vcp < vc) {
            g_membuf[vcp-1] = (uint8_t)sutil_strtohex(verbs[vcp]);
            vcp ++;
        }
        g_memptr = vc - 1;
        // If not already HALTed, do a localized HALT..RUN around the Write operation
        if (is_held || rcmem_hold() == RCM_SUCCESS ) {
            if (rcmem_write(addr, g_membuf, g_memptr, (uint8_t)memtype) != g_memptr) {
                pSendString("WRITE ERROR\r\n");
            } else if (is_held || rcmem_release(NO_CPU_RESET) == RCM_SUCCESS) {
                pSendString("Write OK\r\n");
            } else {
                pSendString("RCU85 release operation failed.\r\n");
            }
        } else {
            pSendString("RCU85 hold operation failed.\r\n");
        }
    } else {
        rc = CMD_ERROR_SYNTAX;
    }
	return rc;
}

static int cmd_addr(int vc, const char * verbs[]) {
    // 1 arg *manditory*
    int rc = CMD_SUCCESS;
    if (vc > 1) {
        rc = CMD_ERROR_SYNTAX;
    } else {
        if (vc == 1 && sutil_ishexstring(verbs[0])) {
            bwAddr = sutil_strtohex(verbs[0]);
            /* update the LED display's address */
            mon_info.ds.dat.addr_hi = (uint8_t)(bwAddr >> 8);
            mon_info.ds.dat.addr_lo = (uint8_t)bwAddr;
        }
        char nbuf[8];
        uint8_t n;
        pSendString("Bulk write addr: 0x");
        n = sutil_asciihex_word(nbuf, bwAddr, SU_NO_PREFIX, SU_LOWERCASE);
        nbuf[n] = '\0';
        pSendString(nbuf);
        pSendString(" (");
        pSendString(s_memtype[memtype]);
        pSendString(")\r\n");
    }
    return rc;
}

typedef enum bwrt_type {
    SBW_HIBYT = 0,
    SBW_LOBYT,
    SBW_COUNT
} bwrt_t;

static bwrt_t bw_state = SBW_HIBYT;

static int cmd_bwrt(int vc, const char * verbs[]) {
    const char valid_hex[] = "0123456789abcdefABCDEF";
    int  rc = CMD_SUCCESS;
    int  cnt;
    char buf[4];
    uint8_t byt, abt;
    bw_state = SBW_HIBYT;
    abt = 0;
    // outer loop - read stdin, up to 4 characters at a time
    while ( !abt && (cnt = preadInputStream(buf, 4, 100)) > 0) {
        while ( cnt > 0 ) {
            if (sutil_strchar(valid_hex,buf[0]) >= 0) {
                if (bw_state == SBW_HIBYT) {
                    byt = sutil_chartohex(buf[0]) << 4;
                    bw_state = SBW_LOBYT;
                } else {
                    byt |= sutil_chartohex(buf[0]);
                    bw_state = SBW_HIBYT;
                    // Program this byte and advance addr pointer.
                    if (rcmem_write(bwAddr, &byt, 1, (uint8_t)memtype) != 1) {
                        pSendString("WRITE ERROR\r\n");
                        abt = 1;
                        break;
                    }
                    bwAddr ++; // set next write byte
                }
            } else if (buf[0] == 0x03) {
                // abort operations 
                abt = 1;
                break;
            } else if (buf[0] != ' ') {
                // invalid character
                rc = CMD_ERROR_SYNTAX;
                abt = 1;
                break;
            }
            // shift left
            buf[0] = buf[1];
            buf[1] = buf[2];
            buf[2] = buf[3];
            buf[3] = 0;
            cnt --;
        }
    }
    return rc;
}

static int cmd_halt(int vc, const char * verbs[]) {
    if ( rcmem_hold() == RCM_SUCCESS ) {
        pSendString("RCU85: HOLD ON\r\n");
    } else {
        pSendString("RCU85 hold operation failed.\r\n");
    }
    return CMD_SUCCESS;
}

static int cmd_run(int vc, const char * verbs[]) {
    int rc = CMD_SUCCESS;
    uint8_t doReset = 0;
    if (vc) {
        if (sutil_strcmp(verbs[0], "reset") == 0) {
            doReset = 1;
        } else {
            rc = CMD_ERROR_SYNTAX;
        }
    }
    if (rc == CMD_SUCCESS) {
        if (rcmem_release(doReset) == RCM_SUCCESS) {
            pSendString("RCU85: RUNNING\r\n");
        } else {
            pSendString("RCU85 run operation failed.\r\n");
        }
    }
    return rc;
}
    
/* Register all commands */
cmdobj pCommandList[10] = {
	{ "help", "h", 0, 0, cmd_help },
    { "mode", "m", 0, 1, cmd_mode },
    { "iom",  "im",0, 1, cmd_iom  },
	{ "read", "r", 2, 2, cmd_read },
    { "write","w", 2, 9, cmd_write},
    { "addr", "a", 0, 1, cmd_addr },
    { "bwrt", "b", 0, 0, cmd_bwrt },
    { "halt","hold",0, 0, cmd_halt },
    { "run", "go", 0, 1, cmd_run  },
	{ NULL, NULL,  0, 0, NULL     }
};
