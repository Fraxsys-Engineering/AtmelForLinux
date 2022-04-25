/*********************************************************************
 * rcu85mem.h
 * 
 * Version 1.0
 * ---
 * RCU85 Memory Manager and API
 * 
 * Command List:
 *  
 **********************************************************************/

#ifndef _RCU85MEM_H_
#define _RCU85MEM_H_

#include <avrlib/avrlib.h>

#define RCM_SUCCESS     0
#define RCM_ERROR       (-1)
#define RCM_TIMEOUT     (-2)

#define SET_MEM_ACCESS  0
#define SET_IO_ACCESS   1

#define NO_CPU_RESET    0
#define RESET_CPU       1

/* Setup for Memory I/O operations -----------------------------------
 * -
 * Configure GPIO and needed memory. 
 * This will initialize GPIO API, if not already done.
 * Returns:     RCM_ERROR, RCM_SUCCESS
 */
int rcmem_init(void);

/* RCU85 in a held state? --------------------------------------------
 * Returns:     (True) "held", (False) "free running"
 */
uint8_t rcmem_isHeld(void);

/* Take over RCU85 bus -----------------------------------------------
 * -
 * This must be called prior to reading or writing the RCU Memory.
 * Call rcmem_release() to return control to the RCU85 CPU and issue
 * an optional reset.
 * Returns:     RCM_ERROR, RCM_SUCCESS
 */
int rcmem_hold(void);

/* Release the RCU85 Bus - Return control to the RCU85 CPU -----------
 * -
 * Arguments:
 *  doReset [bool]  if True, will hold and release reset during the 
 *  memory release event causing the CPU to reboot.
 *  ref. NO_CPU_RESET, RESET_CPU
 * Returns:     RCM_ERROR, RCM_SUCCESS
 */
int rcmem_release(uint8_t doReset);

/* Write a block of data to the RCU85 memory -------------------------
 * -
 * The RCU85 CPU must be in a held state otherwise the call will fail.
 * Refer to RCU85 memory map for valid write locations.
 * Arguments:
 *  addr    [uint16]        starting address for the write, auto-increments.
 *  data    [uint8 *]       buffer holding the write data
 *  len     [uint16]        number of bytes to write
 *  isIO    [bool]          if True, write to I/O memory. Only lower 
 *                          8 bits of address are used.
 *                          ref. SET_MEM_ACCESS, SET_IO_ACCESS
 * Returns:     RCM_ERROR, RCM_SUCCESS
 */
int rcmem_write(uint16_t addr, uint8_t * data, uint16_t len, uint8_t isIO);

/* Read data from the RCU85 memory (RAM, ROM, I/O) -------------------
 * -
 * The RCU85 CPU must be in a held state otherwise the call will fail.
 * Refer to RCU85 memory map for valid write locations.
 * Arguments:
 *  addr    [uint16]        starting address for the read, auto-increments.
 *  data    [uint8 *]       buffer for holding the read data
 *  len     [uint16]        number of bytes to read
 *  isIO    [bool]          if True, read from I/O memory. Only lower 
 *                          8 bits of address are used.
 *                          ref. SET_MEM_ACCESS, SET_IO_ACCESS
 * Returns:     RCM_ERROR, RCM_SUCCESS
 */
int rcmem_read(uint16_t addr, uint8_t * data, uint16_t len, uint8_t isIO);

#endif /* _RCU85MEM_H_ */
