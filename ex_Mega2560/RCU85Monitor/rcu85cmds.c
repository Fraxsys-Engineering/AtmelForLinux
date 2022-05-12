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
  hwrt      [intel hex-data]   Send data in an Intel hex record format. \r\n\
                               No initial address setup is needed but \r\n\
                               memory or I/O space must be set. Mode ends \r\n\
                               when the EOF hex line :00000001FF is sent. \r\n\
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

typedef enum hwrt_state_type {
    HS_WAIT_COLON = 0,      /* wait for start of line (colon :)     */
    HS_WAIT_RECLEN,         /* wait for record len, 1 octet         */
    HS_WAIT_ADDR,           /* wait for address, 4 octets           */
    HS_WAIT_RECTYPE,        /* wait for record type, 1 octet        */
    HS_WAIT_DATA,           /* wait for all data [reclen]           */
    HS_WAIT_CHKSUM,         /* wait for checksum, 1 octet           */
    HS_REC_END,             /* recvd record end                     */
    HS_REC_ABORT            /* error occured, abort parsing and ret */
} hwrt_state_t;

#define RECTYPE_DATA    0x00
#define RECTYPE_EOF     0x01
#define RECTYPE_XSEGADR 0x02
#define RECTYPE_XLINADR 0x04
#define RECTYPE_SLAREC  0x05
#define READ_TMOUT      100

static int cmd_hwrt(int vc, const char * verbs[]) {
    const char valid_chars[] = ":0123456789abcdefABCDEF\r\n";
    char     linebuffer[82];    /* formatted ASCII line */
    uint8_t  databuffer[32];    /* decoded data         */
    int      rc = CMD_SUCCESS;
    int      i,cnt;
    uint8_t  brk = 0;       /* set 'brk' (break) to halt parsing    */
    uint8_t  idx = 0;
    uint8_t  linend  = 0;   /* support partial buffer fills         */
    uint8_t  bufend  = 0;   /* total used buffer length w/ prior frag*/
    uint8_t  reclen  = 0;   /* record length                        */
    uint8_t  rectype = 0;   /* record type                          */
    uint8_t  reccrc  = 0;   /* record crc (decoded from line)       */
    uint8_t  crc     = 0;   /* data crc (calculated)                */
    uint16_t addr    = 0;   /* starting address for 'databuffer'    */
    hwrt_state_t hstate = HS_WAIT_COLON;

    /* Main loop - once entered, keep pulling data from stdin ------*/
    while (hstate < HS_REC_END) {
        cnt = preadInputStream(linebuffer+linend, 80-linend, READ_TMOUT);
        if (cnt > 0) {
            bufend = linend + cnt;
            linebuffer[bufend] = '\0';
            /* check all chars to see that they are valid */
            for ( i = 0 ; i < bufend ; ++i )
                if (sutil_strchar(valid_chars,linebuffer[i]) < 0) {
                    rc = CMD_ERROR_SYNTAX;
                    hstate = HS_REC_ABORT;
                    break;
                }
            if (hstate == HS_REC_ABORT)
                break;
            /* start/resume state machine parsing */
            idx = 0; brk = 0;
            while ( idx < bufend ) {
                switch (hstate) {
                case HS_WAIT_COLON:
                    if (linebuffer[idx] == ':') {
                        hstate = HS_WAIT_RECLEN;
                        pSendString("\r\n:");
                    }
                    idx ++;
                    break;
                case HS_WAIT_RECLEN:
                    /* looking for 2-characters / one octet */
                    if (idx+2 >= bufend || idx+2 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        break;
                    }
                    reclen = (uint8_t)sutil_strntohex(linebuffer+idx,2);
                    pSendChars(linebuffer+idx,2);
                    idx += 2;
                    hstate = HS_WAIT_ADDR;
                    break;
                case HS_WAIT_ADDR:
                    /* looking for 4-characters / two octets */
                    if (idx+4 >= bufend || idx+4 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        break;
                    }
                    addr = sutil_strntohex(linebuffer+idx,4);
                    pSendChars(linebuffer+idx,4);
                    idx += 4;
                    hstate = HS_WAIT_RECTYPE;
                    break;
                case HS_WAIT_RECTYPE:
                    /* looking for 2-characters / one octet */
                    if (idx+2 >= bufend || idx+2 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        break;
                    }
                    rectype = (uint8_t)sutil_strntohex(linebuffer+idx,2);
                    pSendChars(linebuffer+idx,2);
                    idx += 2;
                    hstate = HS_WAIT_DATA;
                    break;
                case HS_WAIT_DATA:
                    /* looking for (reclen * 2) characters / reclen octets */
                    if ((reclen*2)+idx >= bufend || (reclen*2)+idx >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        break;
                    }
                    pSendChars(linebuffer+idx,(reclen*2));
                    /* ready for CRC calculation */
                    crc = reclen + ((uint8_t)(addr >> 8)) + ((uint8_t)(addr&0xff)) + rectype;
                    for ( i = 0 ; i < reclen ; ++i ) {
                        databuffer[i] = (uint8_t)sutil_strntohex(linebuffer+idx+(i*2),2);
                        crc += databuffer[i];
                    }
                    /* add 1 and invert for "2's compliment" */
                    crc = (uint8_t)~crc;
                    crc += 1;
                    idx += (reclen*2);
                    hstate = HS_WAIT_CHKSUM;
                    break;
                case HS_WAIT_CHKSUM:
                    /* looking for 2-characters / one octet */
                    if (idx+2 >= bufend || idx+2 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        break;
                    }
                    reccrc = (uint8_t)sutil_strntohex(linebuffer+idx,2);
                    pSendChars(linebuffer+idx,2);
                    idx += 2;
                    hstate = (crc == reccrc) ? HS_WAIT_COLON : HS_REC_ABORT;
                    /* Write the data if all is good */
                    if ( hstate == HS_WAIT_COLON ) {
                        if (rectype == RECTYPE_DATA) {
                            rc = rcmem_write(addr, databuffer, reclen, (uint8_t)memtype);
                            if (rc != (int)reclen) {
                                hstate = HS_REC_ABORT;
                            } else {
                                pSendString(" OK\r\n");
                            }
                        } else if (rectype == RECTYPE_EOF) {
                            hstate = HS_REC_END;
                            pSendString(" END\r\n");
                        }
                        brk = 1; // do a buffer shift at this point...
                    } else {
                        pSendString(" CRC-ERR\r\n");
                    }
                    break;
                case HS_REC_END:
                case HS_REC_ABORT:
                default:
                    break;
                } /* switch */
                if (brk) {
                    if (hstate == HS_WAIT_COLON) {
                        /* find the next colon, if any */
                        do {
                            if (linebuffer[idx] == ':') {
                                break;
                            }
                            idx ++;
                        } while (idx < bufend);
                    }
                    /* move remaining line to the front */
                    linend = sutil_memcpy(linebuffer, linebuffer+idx, bufend-idx);
                    /* break out to the outer loop, to read stdin */
                    break;
                }
            } /* while (parsing line buffer) */
        } else if (cnt < 0) {
            rc = CMD_ERROR_SYNTAX;
            hstate = HS_REC_ABORT;
        }
        /* a return count of zero is not an error... waiting for human 
         * to copy-paste another hex-rec line into the console... 
         * just keep spinning.
         */
    } /* while (traversing states && reading stdin */
    
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
cmdobj pCommandList[11] = {
	{ "help", "h", 0, 0, cmd_help },
    { "mode", "m", 0, 1, cmd_mode },
    { "iom",  "im",0, 1, cmd_iom  },
	{ "read", "r", 2, 2, cmd_read },
    { "write","w", 2, 9, cmd_write},
    { "addr", "a", 0, 1, cmd_addr },
    { "bwrt", "b", 0, 0, cmd_bwrt },
    { "hwrt", NULL,0, 0, cmd_hwrt },
    { "halt","hold",0, 0, cmd_halt },
    { "run", "go", 0, 1, cmd_run  },
	{ NULL, NULL,  0, 0, NULL     }
};
