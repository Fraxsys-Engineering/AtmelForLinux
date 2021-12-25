/****************************************************************************
 * ioctlcmds.h
 * Driver Command Codes, for IO-CONTROL
 * 
 * This header is open to all.
 * See chardev.h for command descriptions.
 * 
 * Version 1.0
 *
 ***************************************************************************/

#ifndef _IOCTLCMDS_H_
#define _IOCTLCMDS_H_

/* Return Status, when not a file handle */
#define DEV_SUCCESS     0
#define DEV_FAIL      (-1)

/****************************************************************************
 * SERIAL UART
 * 
 *         CMD_RX_PEEK --> (int)n
 *             loads n with the number of Rx bytes waiting to be read
 * 
 *         CMD_TX_PEEK --> (int)n
 *             loads n with the number of bytes free in the Tx buffer
 * 
 *         CMD_RD_BAUD --> (int)n
 *             loads n with the actual (computed) BAUD rate being used in
 *             the UART.
 *
 **************************************************************************/

#define UART_IOCTL_BASE	0x0010
#define CMD_RX_PEEK		(UART_IOCTL_BASE | 0x01)
#define CMD_TX_PEEK		(UART_IOCTL_BASE | 0x02)
#define CMD_RD_BAUD		(UART_IOCTL_BASE | 0x03)





#endif /* _IOCTLCMDS_H_ */
