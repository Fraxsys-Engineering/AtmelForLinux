/*
 * test_cmdparser.c
 *
 * TDD For avrlib/cmdparser.*
 *
 * Supports lib ver: 1.0
 *
 */

#include <avrlib/driver.h>
#include <avrlib/cmdparser.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

// MOCK backend hooks, for testing and inserting character stream
// data, etc.
extern int mock_loop_check_instance(int instance);
extern int mock_loop_write(int instance, const char * buf, int len);
extern int mock_loop_read(int instance, char * buf, int maxlen);
extern int mock_loop_reset(int instance);

//static char test_feedback_buffer[256];

#if 0
static int sutil_strchar(const char * s, const char c) {
    int rc = 0;
    while ( s && *s != '\0' ) {
        if (c == *s)
            break;
        s ++; rc ++;
    }
    return (*s == '\0') ? -1 : rc;
}

static uint16_t sutil_strntohex(char * strn, uint16_t len) {
    static char scopy[8];
    if (len && len < 8) {
        strncpy(scopy, strn, len);
        scopy[len] = '\0';
        return (uint16_t)strtoul(scopy,NULL,16);
    }
    return -1;
}

static int sutil_memcpy(void * to, void * from, int len) {
    memcpy(to, from, len);
    return len;
}
#else
extern int sutil_strchar(const char * s, const char c);
extern uint16_t sutil_strntohex(const char * hexstring, uint16_t len);
extern int32_t sutil_memcpy(void * to, void * from, int len);
#endif

static int rcmem_write(uint16_t addr, uint8_t * data, uint16_t len, uint8_t isIO) {
    int i;
    printf("WRITE (%s) %04x :: ", (isIO)?"IO":"MEM", addr);
    for ( i = 0 ; i < len ; ++i ) {
        printf("%02x ", data[i]);
    }
    printf("\n");
    return len;
}

/* Support testing of the Intel Hex record parser */
static int memtype = 0; /* set to RAM */
#if 0
static char hextest1[] = ":20000000C303003EC0303EFF473D05C20A00FE00C209003E40303EFF473D05C21A00FE0043\r\n:06002000C21900C3030039\r\n:00000001FF\r\n";
static int  ht_len = 0;
static int  ht_ptr = 0;

static int preadInputStream(char * b, int len, int tmout) {
    int blen = 0;
    if (!ht_len)
        ht_len = strlen(hextest1);
    if (b && len && ht_ptr < ht_len) {
        while (blen < len) {
            b[blen] = hextest1[ht_ptr];
            blen ++; ht_ptr++;
            if (ht_ptr >= ht_len) {
                break; /* all written out */
            }
            if (hextest1[ht_ptr] == ':') {
                break; /* buffer a full line at this point */
            }
        } else {
            blen = -1;
        }
    }
    return blen;
}
#endif


/* --- command parser function prototype ------------------------------
typedef int (*cmdparsefcn)(int, const char **);
---------------------------------------------------------------------- */

/* --- command structure ----------------------------------------------
typedef struct cmdobj_type {
    const char * noun;              primary key (noun) long form 
    const char * nsc;               optional noun shortened form 
    unsigned int        verb_min;   minimum # expected verbs     
    unsigned int        verb_max;   maximum # expected verbs     
    cmdparsefcn         cp;         function to call to parse    
} cmdobj;
---------------------------------------------------------------------- */
#define CMD8_DLCOUNT 4
static char cmd8_line0[]   = ":20000000C303003EC0303EFF473D05C20A00FE00";
static char cmd8_line0_1[] = "C209003E40303EFF473D05C21A00FE0043\r\n";
static char cmd8_line1[]   = ":0C00200005C22000FE00C21F00C3070044\r\n";
static char cmd8_line2[]   = ":00000001FF\r\n";
static char * cmd8_data[CMD8_DLCOUNT] = { cmd8_line0, cmd8_line0_1, cmd8_line1, cmd8_line2 };

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

#define READ_TMOUT      20

static int cmd_hwrt(int vc, const char * verbs[]) {
    const char valid_chars[] = ":0123456789abcdefABCDEF\r\n";
    char linebuffer[80];        // formatted ASCII line
    uint8_t databuffer[32];     // decoded data
    int  rc = CMD_SUCCESS;
    int  i,cnt;
    uint8_t  brk = 0;       /* set 'brk' (break) to halt parsing */
    uint8_t  idx = 0;
    uint8_t  linend  = 0;   /* support partial buffer fills         */
    uint8_t  bufend  = 0;   /* total used buffer length w/ prior frag*/
    uint8_t  reclen  = 0;   /* record length                        */
    uint8_t  rectype = 0;   /* record type                          */
    uint8_t  reccrc  = 0;   /* record crc (decoded from line)       */
    uint8_t  crc     = 0;   /* data crc (calculated)                */
    uint16_t addr    = 0;   /* starting address for 'databuffer'    */
    uint8_t  set_lineend = 0; /* TESTING - Set '255' AFTER PARSING ALL DATA LINES */
    hwrt_state_t hstate = HS_WAIT_COLON;
    printf("{cmd_hwrt} -------------------------\n");
    /* SELF-INJECT Line Data */
    printf("    WRITE TO LOOPBACK :: %s", cmd8_data[set_lineend]);
    rc = mock_loop_write(0,cmd8_data[set_lineend],strlen(cmd8_data[set_lineend])); set_lineend++;
    printf("    WRITE TO LOOPBACK, %d chars written.\n", rc);
    /* Main loop - once entered, keep pulling data from stdin ------*/
    while (hstate < HS_REC_END) {
        printf("    preadInputStream(), %d chars max, linend[%d]\n", 80-linend, linend);
        cnt = preadInputStream(linebuffer+linend, 80-linend, READ_TMOUT);
        if (cnt > 0) {
            linebuffer[ linend+cnt ] = '\0';
        }
        printf("    {stdin} len[%d] :: %s\n", cnt, (cnt>0)?linebuffer:"NIL");
        if (cnt > 0) {
            bufend = linend + cnt;
            printf("    linend[%d] bufend[%d]\n", linend, bufend);
            /* check all chars to see that they are valid */
            for ( i = 0 ; i < bufend ; ++i )
                if (sutil_strchar(valid_chars,linebuffer[i]) < 0) {
                    rc = CMD_ERROR_SYNTAX;
                    hstate = HS_REC_ABORT;
                    printf("    bad char! :: %c\n", linebuffer[i]);
                    break;
                }
            if (hstate == HS_REC_ABORT)
                break;
            /* start/resume state machine parsing */
            idx = 0; brk = 0;
            printf("    parser ... \n");
            while ( idx < bufend ) {
                printf("    state-machine hstate[%d] ... \n", hstate);
                switch (hstate) {
                case HS_WAIT_COLON:
                    if (linebuffer[idx] == ':') {
                        hstate = HS_WAIT_RECLEN;
                        printf("    found linestart...\n");
                    }
                    idx ++;
                    break;
                case HS_WAIT_RECLEN:
                    /* looking for 2-characters / one octet */
                    if (idx+2 >= bufend || idx+2 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        printf("    WAIT_RECLEN : insufficient remaining line\n");
                        break;
                    }
                    reclen = (uint8_t)sutil_strntohex(linebuffer+idx,2);
                    idx += 2;
                    hstate = HS_WAIT_ADDR;
                    printf("    reclen[%d]\n", reclen);
                    break;
                case HS_WAIT_ADDR:
                    /* looking for 4-characters / two octets */
                    if (idx+4 >= bufend || idx+4 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        printf("    WAIT_ADDR : insufficient remaining line\n");
                        break;
                    }
                    addr = sutil_strntohex(linebuffer+idx,4);
                    idx += 4;
                    hstate = HS_WAIT_RECTYPE;
                    printf("    addr[0x%04x]\n", addr);
                    break;
                case HS_WAIT_RECTYPE:
                    /* looking for 2-characters / one octet */
                    if (idx+2 >= bufend || idx+2 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        printf("    WAIT_RECTYPE : insufficient remaining line\n");
                        break;
                    }
                    rectype = (uint8_t)sutil_strntohex(linebuffer+idx,2);
                    idx += 2;
                    hstate = HS_WAIT_DATA;
                    printf("    rectype[%d]\n", rectype);
                    break;
                case HS_WAIT_DATA:
                    /* looking for (reclen * 2) characters / reclen octets */
                    if ((reclen*2)+idx >= bufend || (reclen*2)+idx >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        printf("    WAIT_DATA : insufficient remaining line\n");
                        /* TESTING *** inject another stdin line, for more data arriving */
                        mock_loop_write(0,cmd8_data[set_lineend],strlen(cmd8_data[set_lineend])); set_lineend++;
                        break;
                    }
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
                    printf("    data parsing - calculated crc[0x%02x]\n", crc);
                    break;
                case HS_WAIT_CHKSUM:
                    /* looking for 2-characters / one octet */
                    if (idx+2 >= bufend || idx+2 >= 80) {
                        // TODO pause parsing, rotate linebuffer and get more data
                        brk = 1;
                        printf("    WAIT_CHKSUM : insufficient remaining line\n");
                        break;
                    }
                    reccrc = (uint8_t)sutil_strntohex(linebuffer+idx,2);
                    idx += 2;
                    printf("    line crc[0x%02x]\n", reccrc);
                    hstate = (crc == reccrc) ? HS_WAIT_COLON : HS_REC_ABORT;
                    /* Write the data if all is good */
                    if ( hstate == HS_WAIT_COLON ) {
                        printf("    hstate[HS_WAIT_COLON]\n");
                        if (rectype == RECTYPE_DATA) {
                            printf("    rectype[RECTYPE_DATA] >> *** WRITE MEMORY ***\n");
                            rc = rcmem_write(addr, databuffer, reclen, (uint8_t)memtype);
                            if (rc != (int)reclen) {
                                pSendString("RCU85: Bus write failure\r\n");
                                hstate = HS_REC_ABORT;
                            } else if (set_lineend < CMD8_DLCOUNT) {
                                mock_loop_write(0,cmd8_data[set_lineend],strlen(cmd8_data[set_lineend])); set_lineend++;
                            } else {
                                printf("    **all data sent!\n");
                            }
                        } else if (rectype == RECTYPE_EOF) {
                            hstate = HS_REC_END;
                            printf("    rectype[RECTYPE_EOF] >> hstate := RECTYPE_EOF\n");
                        }
                        brk = 1; // do a buffer shift at this point...
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

static int cmd_foo(int vc, const char * verbs[]) {
	char vb1[20];
	strcpy(vb1,"<nil>");
	
	pSendString("cmd_foo");
    if (vc == 1) {
		strcpy(vb1,verbs[0]);
        pSendString("/");
        pSendString(vb1);
	}
    pSendString("\r\n");
	printf("(cmd_foo) vc:%d v1:%s\n", vc, vb1);
	return CMD_SUCCESS;
}

static int cmd_bar(int vc, const char * verbs[]) {
	printf("(cmd_bar) vc:%d\n", vc);
    pSendString("cmd_bar\r\n");
	return CMD_SUCCESS;
}

cmdobj pCommandList[4] = {
	{ "foo", NULL,  0, 1, cmd_foo},
	{ "bar", "b",   0, 0, cmd_bar},
    { "hwrt", NULL, 0, 0, cmd_hwrt},
	{ NULL, NULL,   0, 0, NULL}
};

static int StringContains(const char * strn, const char * has) {
    return (strn && has && strstr(strn,has) != NULL) ? 1 : 0;
}

// ======== TEST SUITE ================================================

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void test_cmdparser(void) {
    int  len;
	int  inst = 0;
	char rxb[128];

    char pHeader[] = "AVRLIB Command Parser Version 1.0\r\n";
    char cmdbad[] = "bart\r\n";
    char cmd1[] = "foo\r\n";
    char cmd1_resp[] = "cmd_foo";
    char cmd2[] = "foo 3\r\n";
    char cmd2_resp[] = "cmd_foo/3";
    char cmd3[] = "bar badverb\r\n";
    char cmd4[] = "b\r\n";
    char cmd4_resp[] = "cmd_bar";
    char cmd5[] = "foo 3 4\r\n";
    char cmd8[] = "hwrt\r\n";
    char resp_ok[] = "\r\nOK\r\n";
    char resp_err[] = "Error (";

	printf("\n"); // CUnit annoyingly does not CR when adding a print stating that it is starting this test...
	
	sprintf(rxb,"/dev/loop/%d",inst);
	printf("{test_cmdparser} Opening device: %s on parser...\n", rxb);
	CU_ASSERT( startParser(rxb, 0) == 0);
	
	// make sure loopback instance <inst> was created.
	CU_ASSERT_FATAL( mock_loop_check_instance(inst) == 1 );

    // [1] loopback should be holding the Parser "login" header 'pHeader'
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Parser Header :: %s", rxb);
    CU_ASSERT_STRING_EQUAL_FATAL(rxb,pHeader);
	
    // [2] Test invalid command
    
    printf("\n***[2]***\n");
    mock_loop_write(inst,cmdbad,strlen(cmdbad));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,resp_err) );

    // [3] command w/ no parameters (verbs)
    
    printf("\n***[3]***\n");
    mock_loop_write(inst,cmd1,strlen(cmd1));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,cmd1_resp) );
    CU_ASSERT_FATAL( StringContains(rxb,resp_ok) );
    
    // [4] command w/ verbs
    
    printf("\n***[4]***\n");
    mock_loop_write(inst,cmd2,strlen(cmd2));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,cmd2_resp) );
    CU_ASSERT_FATAL( StringContains(rxb,resp_ok) );
    
    // [5] send verbs to a command that does not accept them
    
    printf("\n***[5]***\n");
    mock_loop_write(inst,cmd3,strlen(cmd3));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,resp_err) );
    
    // [6] command short form
    
    printf("\n***[6]***\n");
    mock_loop_write(inst,cmd4,strlen(cmd4));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,cmd4_resp) );
    CU_ASSERT_FATAL( StringContains(rxb,resp_ok) );
    
    // [7] too many verbs (send 2 to 'foo')
    
    printf("\n***[7]***\n");
    mock_loop_write(inst,cmd5,strlen(cmd5));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,resp_err) );
    
    // [8] Test parsing of Intel Hex Strings
    printf("\n***[8]***\n");
    mock_loop_write(inst,cmd8,strlen(cmd8));
    
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    
    // just exit, parser cannot be closed...

}

int main() {
	// char strean driver stack - system init
	printf("{TDD} Driver Stack Init...\n");
	printf("{TDD} Registering the loopback driver...\n");
	System_DriverStartup();

	printf("{TDD} Initializing the Drivers...\n");
	System_driverInit();

    CU_pSuite pSuite = NULL;
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
        
    pSuite = CU_add_suite("Test Suite - stringutils", init_suite, clean_suite);
    if (pSuite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ( !CU_add_test(pSuite, "test cmdparser / foo & bar", test_cmdparser) ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

