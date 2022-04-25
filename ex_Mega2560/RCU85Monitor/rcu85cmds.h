/*********************************************************************
 * rcu85cmds.h
 * 
 * Version 1.0
 * ---
 * Commands for the RCU85 Monitor
 * 
 * Command List:
 *  
 **********************************************************************/

#ifndef _RCU85CMDS_H_
#define _RCU85CMDS_H_

/* send the welcome header text */
void rcmd_sendWelcome(void);

/* send the help menu */
void rcmd_sendHelpMenu(void);

/* === COMMAND API ===================================================*/

typedef enum mon_state_type {
    MSTATE_KYBD = 0,
    MSTATE_TERM,
    /* ... */
    MSTATE_COUNT
} mon_state_t;

// Read the current monitor state. This determines the data source
// for updating the LEd display.
mon_state_t rcmd_readMonitorState(void);



#endif /* _RCU85CMDS_H_ */
