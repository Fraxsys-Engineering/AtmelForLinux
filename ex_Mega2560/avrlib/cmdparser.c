/****************************************************************************
 * cmdparser.c
 * API and hooks for a simple command parser.
 * 
 * Version 1.0
 *
 ***************************************************************************/

#include "stringutils.h"
#include "libtime.h"
#include "cmdparser.h"
#include "chardev.h"

#ifndef P_MAX_VERBCOUNT
#define P_MAX_VERBCOUNT 2
#endif

#ifndef P_MAX_VERBLEN
#define P_MAX_VERBLEN   16
#endif

#ifndef P_MAX_CMDLEN
#define P_MAX_CMDLEN    80
#endif

#ifndef TEMP_BUF_LEN
#define TEMP_BUF_LEN    64
#endif

static const char sInit[] = "AVRLIB Command Parser Version 1.0";
static const char sErrVerbCount[] = "Error (invalid arg count)\r\n";
static const char sErrFail[]      = "Error (command fail)\r\n";
static const char sErrUnknown[]   = "Error (unknown command)\r\n";
static const char sErrCmdSyntax[] = "Error (command syntax)\r\n";
static const char sErrInternal[]  = "Error (internal)\r\n";

//                            CMD_FAIL  CMD_ERROR_UNKNOWN CMD_ERROR_SYNTAX CMD_POLL_FAIL
const char * sCmdErrList[] = {sErrFail, sErrUnknown,      sErrCmdSyntax,   sErrInternal};

char   verblist[P_MAX_VERBCOUNT][P_MAX_VERBLEN+2];
char   noun[P_MAX_VERBLEN+2];
char * verbv[P_MAX_VERBCOUNT];
int    verbcount = 0;
char   cmdbuffer[P_MAX_CMDLEN+2];
char * cb = &(cmdbuffer[0]); // is changed while cmdstring is parsed.
int    cmdptr = 0;
int    serdesc = DEV_FAIL;
char   tempbuf[TEMP_BUF_LEN];

// busy loop-spin until all characters pushed into the transmit buffer.
int pSendString(const char * text) {
    int rc = CMD_FAIL;
    if (serdesc > 0) {
        int len = strlen(text);
        int remlen = len;
        int sp = 0;
        int sent = cd_write(serdesc, text+sp, remlen);
        while (sent >= 0 && sp < len) {
            sp += sent;
            remlen -= sp;
            tm_delay_ms(10);
            sent = cd_write(serdesc, text+sp, remlen);
        }
        rc = (sent >=0) ? DEV_SUCCESS : DEV_FAIL;
    }
    return rc;
}

int pSendHexByte(const uint8_t val) {
    int rc = CMD_FAIL;
    if (serdesc > 0) {
        uint8_t sp = sutil_asciihex_byte(tempbuf, val, 0, 0);
        tempbuf[sp] = '\0';
        rc = pSendString(tempbuf);
    }
    return rc;
}

int pSendHexShort(const uint16_t val) {
    int rc = CMD_FAIL;
    if (serdesc > 0) {
        uint8_t sp = sutil_asciihex_word(tempbuf, val, 0, 0);
        tempbuf[sp] = '\0';
        rc = pSendString(tempbuf);
    }
    return rc;
}

int pSendHexLong(const uint32_t val) {
    int rc = CMD_FAIL;
    if (serdesc > 0) {
        uint8_t sp = sutil_asciihex_long(tempbuf, val, 0, 0);
        tempbuf[sp] = '\0';
        rc = pSendString(tempbuf);
    }
    return rc;
}

int pSendInt(const int32_t val) {
    int rc = CMD_FAIL;
    if (serdesc > 0) {
        uint8_t sp = sutil_asciinumber(tempbuf, val);
        tempbuf[sp] = '\0';
        rc = pSendString(tempbuf);
    }
    return rc;
}

int pollParser(void) {
    int rc = CMD_FAIL;
    if (serdesc > 0) {
        /* Build the command buffer with incoming data until 
         * crlf received.
         */
        rc = cd_read(serdesc, cmdbuffer+cmdptr, (P_MAX_CMDLEN-cmdptr));
        if (rc > 0) {
            cmdptr += rc;
            if (cmdptr && (cmdbuffer[cmdptr-1] == '\r' || cmdbuffer[cmdptr-1] == '\n')) {
                // pull out the noun, then any following verbs
                int i;
                char * n = sutil_strtok(cb, " \r\n");
                if (n) {
                    if (sutil_strlen(cb) > P_MAX_VERBLEN)
                        cb[P_MAX_VERBLEN] = '\0'; // TRUNCATE!
                    sutil_strcpy(noun,cb);
                }
                // pull out any verbs
                verbcount = 0;
                while (n) {
                    cb = n;
                    n = sutil_strtok(cb, " \r\n");
                    if (n) {
                        verbv[verbcount++] = cb;
                    }
                }
                // look for the command
                i = 0;
                while (pCommandList[i]) {
                    if (sutil_strcmp(noun, pCommandList[i]->noun) == 0 || 
                        sutil_strcmp(noun, pCommandList[i]->nsc) == 0  ) {
                        // -- COMMAND MATCH
                        if (verbcount < pCommandList[i]->verb_min ||
                            verbcount > pCommandList[i]->verb_max ) {
                            // Syntax Error (mis-matching verb count)
                            pSendString(sErrVerbCount);
                            break;
                        }
                        i = pCommandList[i]->cp(verbcount, (const char **)verbv);
                        if (i < 0) {
                            // invert error and use to lookup error code
                            i = i * -1;
                            i = i - 1;
                            pSendString(sCmdErrList[i]);
                        }
                        break;
                    }
                    i ++;
                }
                // when cleaning up after cmd parsing, reset the command
                // buffer pointers and clean up the buffer.
                cb = &(cmdbuffer[0]);
                cmdptr = 0;
                rc = CMD_SUCCESS;
            }
        } else if (rc < 0) {
            rc = CMD_POLL_FAIL;
        }
    }
    return rc;
}

int startParser(const char * serialdev. int mode) {
    int rc = CMD_FAIL;
    if (serdesc <= 0) {
        serdesc = cd_open(serialdev, mode);
        if (serdesc > 0) {
            cmdptr = 0; /* reset back to the cmd buffer start */
            pSendString(sInit);
            rc = CMD_SUCCESS;
        }
    }
    return rc;
}

