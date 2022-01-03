/****************************************************************************
 * cmdparser.h
 * API and hooks for a simple command parser.
 * 
 * Version 1.0
 *
 * ESTABLISHMENT OF COMMAD SYNTAX
 * 
 * Commands consist of a simple set of keys. The primary key (noun) is
 * followed by zero or more verbs, up to a fixed limit. The noun is 
 * static but verbs can be variable strings or numbers. Verbs are passed
 * to the command parser for further processing.
 * Nouns can be given an optional shortcut equivalent.
 * Verbs are passed into the parser in an arglist of string pointers.
 * 
 * Commands are registered in an array of structs of the form:
 * 
 * struct cmdobj {
 *     static const char * noun;        primary key (noun) long form
 *     static const char * nsc;         optional noun shortened form
 *     unsigned int        verb_min;    minimum # expected verbs
 *     unsigned int        verb_max;    maximum # expected verbs
 *     cmdparsefcn         cp;          function to call to parse
 * };
 *
 * On a noun or short-form match, the corresponding 'cp' function is
 * called with the collected verbs. 
 * 
 * The user code must create a list of commands with the variable name:
 * pCommandList
 * which the parser is expecting to be defined.
 * The list can be terminated with a "null" command that contains a
 * 'noun' set to NULL. List iteration stops when a NULL noun is found 
 * in the list.
 * 
 * CMD PARSER OUTPUT AND RETURN
 *
 * The command parse function has the form:
 * 
 * typedef int (*cmdparsefcn)(int, const char *);
 * 	arg1:  	verbc (# of verbs in verbv)
 *  arg2:  	verbv (# of parsed verbs)
 *  return:	-n (error), 0 (SUCCESS)
 *  
 * The called command parse function 'cp' can return a negative value
 * to report a syntax or other error allowing the parser to report back
 * the error to the serial terminal. Success status is 0 which generates
 * an "OK" response.
 * During the command parsing it can generate additional string data 
 * that can be sent back to the serial terminal using an API call in
 * this parser.
 * 
 * USER ESTABLISHED LIMITS
 * 
 * User is to set these definitions to establish limits on parsing:
 * 
 *  P_MAX_VERBCOUNT     the maximum verb count accross all commands
 *  P_MAX_VERBLEN       the longest verb in any command
 *  P_MAX_CMDLEN        maximum characters to buffer for set of all 
 *                      commands.
 * 
 ***************************************************************************/

#ifndef _CMDPARSER_H_
#define _CMDPARSER_H_

#include <stdint.h>

typedef enum cmdStatus_type {
    CMD_FAIL          = -1,
    CMD_ERROR_UNKNOWN = -2,
    CMD_ERROR_SYNTAX  = -3,
    CMD_POLL_FAIL     = -4,
    CMD_SUCCESS       = 0
} cndStatus_t;

typedef int (*cmdparsefcn)(int, const char **);

typedef struct cmdobj_type {
    static const char * noun;       /* primary key (noun) long form */
    static const char * nsc;        /* optional noun shortened form */
    unsigned int        verb_min;   /* minimum # expected verbs     */
    unsigned int        verb_max;   /* maximum # expected verbs     */
    cmdparsefcn         cp;         /* function to call to parse    */
} cmdobj;

/* Commands can return these errors and a success */
typedef enum cmdstatus_type {
    PCMD_ERR_SYNTAX = -1,
    PCMD_SUCCESS = 0
} cmdstatus;

extern cmdobj * const pCommandList[];

/* Commands can call these to feed data back to the terminal during 
 * processing.
 * (N) String data has to be EOL ('\0') terminated.
 * (N) Functions WILL NOT add "\r\n" to the end of strings! You must do this.
 * (!) Calls will block if TX Buffers are full and return only when all
 *     data has been transferred to the buffer.
 */
int pSendString(const char * text);
int pSendHexByte(const uint8_t val);
int pSendHexShort(const uint16_t val);
int pSendHexLong(const uint32_t val);
int pSendInt(const int32_t val);

/* User main loop should call this to poll the command parser. It 
 * blocks until a received command is completed.
 * Return: 0 on success
 */
int pollParser(void);

/* User to start the parser by openeing serial device and setting up
 * parsing operations.
 * Return: 0 on success, -1 on failure.
 */
int startParser(const char * serialdev, int mode);

#endif /* _CMDPARSER_H_ */

