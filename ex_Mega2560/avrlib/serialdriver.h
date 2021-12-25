/****************************************************************************
 * serialdriver.h ** DEPRECATED - Use chardev.h to open UART driver**
 * Simple inteerupt driver serial driver with buffering. Requires additional
 * layer for I/O character stream support (not included in this code).
 * Version 1.0
 * ---
 * REQUIREMENTS
 * [1.0] 	Clocking
 *  [1.1]	Compiler Makefile MUST define F_CPU which is the external
 *          crystal frequency in Hz. For "Mega2560" boards is is normally
 *          16 MHz (16000000UL).
 * PHASES
 * [A]		Simple API, fixed UART, normal speed async only
 * [B]      Convert to driver f-ptrs + dev struct + buffers & bkground ISR
 * [C]      Wrap into a defined set of drivers. Top level API to mount
 *          correct drivers using names, etc. "/dev/uart1"
 *          open: "/dev/uart1,9600,8n1" 		(no flow control)
 *                "/dev/uart1,9600,8n1,rtscts" 	(H/W flow control)
 ***************************************************************************/

//#ifndef _SERIALDRIVER_H_
//#define _SERIALDRIVER_H_
#if 0

#include <stdint.h>  // uint8_t etc.


//Phase [A]

// Define the character count for a simple PUBLIC serial buffer. This can be
// examined by the caller! later phases will make this buffer private.

#ifndef SERBUF_MAX_LEN
#define SERBUF_MAX_LEN 256
#endif

extern uint8_t serbuf_wr[SERBUF_MAX_LEN];
extern uint8_t serbuf_rd[SERBUF_MAX_LEN];
// A simple circular buffer is instantiated on top of this buffer...
extern uint16_t serbuf_wr_head; // where user can write new data
extern uint16_t serbuf_wr_tail; // where driver is pulling data from for Tx serial output
extern uint16_t serbuf_wr_used; // amount used out of total
extern uint16_t serbuf_wr_free; // amount free out of total
// -
extern uint16_t serbuf_rd_head; // where driver can write new data from Rx serial input
extern uint16_t serbuf_rd_tail; // where user is pulling data from
extern uint16_t serbuf_rd_used; // amount used out of total
extern uint16_t serbuf_rd_free; // amount free out of total

typedef enum serialparity_type {
	SP_NONE = 0,	// no parity (normal)
	SP_EVEN,		// even parity
	SP_ODD,			// odd parity
	/* End of valid enums */
	SP_COUNT
} sparity_t;

typedef enum serialframebits_type {
	SF_5 = 0,		// 5 data bits
	SF_6,			// 6 data bits
	SF_7,			// 7 data bits
	SF_8,			// 8 data bits (normal)
	SF_9,			// 9 data bits (?)
	/* End of valid enums */
	SF_COUNT
} sfbits_t;

typedef enum serialstopbits_type {
	SS_1 = 0,		// 1 stop bit (normal)
	SS_2,			// 2 stop bits 
	/* End of valid enums */
	SS_COUNT
} ssbits_t;

typedef enum serialuartmode_type {
	SM_ASYNC = 0,	// Asynchronous UART - PHASE[A] ONLY ONE SUPPORTED !
	SM_SYNC_TCR,	// Synchronous USART, Tx bit changed on CLK rising edge, Rx sampled on CLK falling edge
	SM_SYNC_TCF,	// Synchronous USART, Tx bit changed on CLK falling edge, Rx sampled on CLK rising edge
	SM_M_SPI,		// Master SPI (MSPIM)
	/* End of valid enums */
	SM_COUNT
} sumode_t;

// Open and initialize a new interface.
//	baud	desired baudrate (9600,38400,57600,115200 etc.)
//	frame	# data bits (SF_5 .. SF_9) => (5,6,7,8,9)
//	stops	# stop bits (SS_1, SS_2)   => (1,2)
//	par		parity  (none, even, odd)
//	Returns: {-1,0} 0 := success
//		May fail if baud rate is not possible
extern int serial_open(uint32_t baud, sfbits_t frame, ssbits_t stops, sparity_t par);

// debugging - return the actual calculated baud rate
//	baud	if non-zero, then this value is calculated. If zero then the
//			computed baud from the newly opened port is returned. If the
//			serial port is closed then (uint32_t)-1 is returned.
extern uint32_t serial_realbaud(uint32_t baud);

// Erase any data in the buffers and reset them
extern int serial_reset(void);

// [USER] Write out data to serial port (Tx)
//	strn 	buffer holding string data to copy and buffer
//  len     length of string. do not expect a terminating '\0' char
//  Returns: 
//		-n		Error
//		0		nothing sent (tx buffer full)
//		+n		n characters sent (may be a partial transfer). Roll string forward by n and retry later.
extern int serial_puts(const char * strn, int len);

// Same as above but expects a '\0' terminated string and generates len(gth) internally.
extern int serial_putstrn(const char * strn);

// [USER] Check to see if there is any data waiting to be read, from incoming Rx serial.
// 	Returns: 
//		-n		Error
//		0		no data
//		+n		n characters received. If greater than SERBUF_MAX_LEN than some data was dropped!
extern int serial_rd_peek(void);

// [USER] Same as the rd_peek() except this checks the Tx FIFO to see 
//        how much space there is available for writing into. For cases
//        where the user wants to write the entire buffer into the FIFO
//        and not hang about doing partial writes.
extern int serial_wr_peek(void);

// [USER] Read data arriving from serial port
//	strn 	buffer for holding string data from buffer
//  maxread max. charas that can be transferred into 'strn'
//  Returns: 
//		-n		Error
//		0		Nothing to read
//		+n		n chars copied in, up to 'maxread'
extern int serial_gets(char * strn, int maxread);

// shut the driver down and release the IO pins from duty as UART. Stop all interrupts.
extern int serial_close(void);


#endif /* _SERIALDRIVER_H_ */
