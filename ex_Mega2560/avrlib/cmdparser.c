/****************************************************************************
 * cmdparser.c
 * API and hooks for a simple command parser.
 * 
 * Version 1.0
 *
 ***************************************************************************/

#include "stringutils.h"
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

static const char sInit[] = "Command Parser Version 1.0";

char verblist[P_MAX_VERBCOUNT][P_MAX_VERBLEN+2];
char noun[P_MAX_VERBLEN+2];
char * verbv[P_MAX_VERBCOUNT];
char cmdbuffer[P_MAX_CMDLEN+2];
static int cmdptr = 0;
static int serdesc = DEV_FAIL;
static char tempbuf[TEMP_BUF_LEN];

/* Extract a word (nown, verb) out of the command buffer.
 * If the end of the command buffer is reached then the function 
 * returns NULL 
 */
static char * pullword(char * cmd, char * nvbuf, int maxnvlen) {
    int iter = 0;
    while (cmd[iter] != ' ' && cmd[iter] != '\r' && cmd[iter] != '\n' && iter < maxnvlen) {
        *nvbuf = cmd[iter];
        nvbuf ++;
        iter ++;
    }
    *nvbuf = '\0';
    return (iter) ? cmd+iter+1 : NULL;
}

int pSendString(const char * text) {
    int rc = DEV_FAIL;
    if (serdesc > 0) {
    }
    return rc;
}

int pSendHexByte(const uint8_t val) {
    int rc = DEV_FAIL;
    if (serdesc > 0) {
    }
    return rc;
}

int pSendHexShort(const uint16_t val) {
    int rc = DEV_FAIL;
    if (serdesc > 0) {
    }
    return rc;
}

int pSendHexLong(const uint32_t val) {
    int rc = DEV_FAIL;
    if (serdesc > 0) {
    }
    return rc;
}

int pSendInt(const int val) {
    int rc = DEV_FAIL;
    if (serdesc > 0) {
    }
    return rc;
}

int pollParser(void) {
    int rc = DEV_FAIL;
    if (serdesc > 0) {
        /* Build the command buffer with incoming data until 
         * crlf received.
         */
        rc = read(serdesc, cmdbuffer+cmdptr, (P_MAX_CMDLEN-cmdptr));
        if (rc > 0) {
            cmdptr += rc;
            if (cmdptr && (cmdbuffer[cmdptr-1] == '\r' || cmdbuffer[cmdptr-1] == '\n')) {
                // pull out the noun, then any following verbs
                
            
            
            }
        }
        rc = DEV_SUCCESS;
    }
    return rc;
}

int startParser(const char * serialdev. int mode) {
    int rc = DEV_FAIL;
    if (serdesc <= 0) {
        serdesc = open(serialdev, mode);
        if (serdesc > 0) {
            cmdptr = 0; /* reset back to the cmd buffer start */
            pSendString(sInit);
            rc = DEV_SUCCESS;
        }
    }
    return rc;
}

