/****************************************************************************
 * serialdriver.c
 * Simple inteerupt driver serial driver with buffering. Requires additional
 * layer for I/O character stream support (not included in this code).
 * Version 1.0 (Phase A)
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

#include "serialdriver.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>  // string ops.

//Debugging Only
//#include "dblink.h"

typedef enum serial_state_type {
	USS_CLOSED  = 0,
	USS_IDLE    = 0x01,
	USS_READING = 0x02,
	USS_WRITING = 0x04
	/* end valid values */
} uss_t;

//typedef struct serial_state_type {
//	uss_t	state;
//	
//} serstate_t;

static uss_t serial_state = USS_CLOSED;
static uint8_t serial_frame;
static uint8_t serial_stops;
static sparity_t serial_par;

static uint16_t last_ubrr = 0;
static uint32_t real_baud = 0;

uint8_t serbuf_wr[SERBUF_MAX_LEN];
uint8_t serbuf_rd[SERBUF_MAX_LEN];
uint8_t voidbyte;  // on errors, read data to this variable to clear flags. this data is ignored.

uint16_t serbuf_wr_head = 0; // where user can write new data
uint16_t serbuf_wr_tail = 0; // where driver is pulling data from for Tx serial output
uint16_t serbuf_wr_used = 0; // amount used out of total
uint16_t serbuf_wr_free = SERBUF_MAX_LEN; // amount free out of total

uint16_t serbuf_rd_head = 0; // where driver can write new data from Rx serial input
uint16_t serbuf_rd_tail = 0; // where user is pulling data from
uint16_t serbuf_rd_used = 0; // amount used out of total
uint16_t serbuf_rd_free = SERBUF_MAX_LEN; // amount free out of total
uint32_t serbuf_rd_overflow = 0; // count # bytes that were missed due to a full Rx buffer

// Serial Port Selection, Phase [A] statically defined
#define SER_FIFO    UDR1
#define SER_CTSR_A  UCSR1A
#define SER_CTSR_B  UCSR1B
#define SER_CTSR_C  UCSR1C
#define SER_UBRR_H	UBRR1H
#define SER_UBRR_L	UBRR1L
#define SER_UBRR_H_MASK 0x0F  /* upper 4 bits must be written as zero */
// Pre-shifted bits (all bit offsets the same for each uart)
#define MSK_RXCIE	0x80		/* [UCSRnB] Rx Complete INT Enable */
#define MSK_TXCIE	0x40		/* [UCSRnB] Tx Complete INT Enable*/
#define MSK_UDRIE	0x20		/* [UCSRnB] UART Data Register Empty INT Enable */
#define MSK_RXEN	0x10		/* [UCSRnB] RX Enable */
#define MSK_TXEN	0x08		/* [UCSRnB] TX Enable */
#define MSK_RXB8	0x02		/* [UCSRnB] Rx Data Bit 8 (frame=9) */
#define MSK_TXB8	0x01		/* [UCSRnB] Tx Data Bit 8 (frame=9) */

// ====== MACROS To Enable/Disable Interrupt Service Routines =========
// [Rx] Read FIFO, Rx FIFO FULL
#define ENABLE_INTR_RECV_COMPLETE()     SER_CTSR_B |= MSK_RXCIE;
#define DISABLE_INTR_RECV_COMPLETE()    SER_CTSR_B &= (uint8_t)~(MSK_RXCIE);
// [Tx] Write FIFO, Tx FIFO EMPTY
#define ENABLE_INTR_TR_FIFO_EMPTY()     SER_CTSR_B |= MSK_UDRIE;
#define DISABLE_INTR_TR_FIFO_EMPTY()    SER_CTSR_B &= (uint8_t)~(MSK_UDRIE);
// [Tx] Tx Shifter Complete & Tx FIFO EMPTY
#define ENABLE_INTR_TR_COMPLETE()       SER_CTSR_B |= MSK_TXCIE;
#define DISABLE_INTR_TR_COMPLETE()      SER_CTSR_B &= (uint8_t)~(MSK_TXCIE);

// Wrap atomic code around this - future: replace with <util.atomic.h> ?
#define __ENTER_CRITICAL_SECTION__()   cli();
#define __EXIT_CRITICAL_SECTION__()    sei();

// Pre-computes bit tables for frame, stops etc.
static const uint8_t smode_bits[SM_COUNT] = {
	0x00, 0x40, 0x41, 0xC0				// [UCSRnC] - async, sync_tcr, sync_tcf, M.SPI
};
// [n][0] := UCSRnC, Zn0, Zn1
// [n][1] := UCSRnB, Zn2
static const uint8_t frame_bits[SF_COUNT][SS_COUNT] = {
	{0x00, 0x00},	// [UCSRnC],[UCSRnB] 5
	{0x02, 0x00},	// [UCSRnC],[UCSRnB] 6
	{0x04, 0x00},	// [UCSRnC],[UCSRnB] 7
	{0x06, 0x00},	// [UCSRnC],[UCSRnB] 8
	{0x06, 0x04}	// [UCSRnC],[UCSRnB] 9
};
static const uint8_t parity_bits[SP_COUNT] = {
	0x00, 0x20, 0x30					// [UCSRnC] - none,even,odd
};
static const uint8_t stops_bits[2] = {
	0x00, 0x08							// [UCSRnC] - 1, 2
};

static uint16_t calc_ubrr( uint32_t baud ) {
	// [ F_CPU / (16 x BAUD) ] - 1
	uint32_t lval = F_CPU;
	lval = lval / 16;
	lval = lval / baud;
	last_ubrr = (uint16_t)(lval - 1);
	// F_CPU / [ 16 x (UBRRn + 1) ]
	lval = 16 * (last_ubrr + 1);
	real_baud = (uint32_t)F_CPU / lval;
	return last_ubrr;
}

int serial_open(uint32_t baud, sfbits_t frame, ssbits_t stops, sparity_t par) {
	int rc = -1; // assume something went wrong
	if (serial_state == USS_CLOSED && frame < SF_COUNT && stops < SS_COUNT && par < SP_COUNT) {
		calc_ubrr(baud);
		// Register Setup
		SER_UBRR_H = ((uint8_t)(last_ubrr >> 8)) & SER_UBRR_H_MASK;
		SER_UBRR_L = (uint8_t)last_ubrr;
		SER_CTSR_A = 0;
		SER_CTSR_C = smode_bits[SM_ASYNC] | parity_bits[par] | stops_bits[stops] | frame_bits[frame][0];
		SER_CTSR_B = MSK_RXEN | MSK_TXEN | frame_bits[frame][1];
		serial_frame = frame;
		serial_stops = stops;
		serial_par   = par;
		serial_state = USS_IDLE;
		serial_reset();
		ENABLE_INTR_RECV_COMPLETE(); // Enable receiver only. Tx will be started when it gets something to send.
#if 1
		sei(); // enable global interrupts, if disabled.
#endif
		rc = 0;
	}
	return rc;
}

uint32_t serial_realbaud(uint32_t baud) {
	uint32_t _last_ubrr = last_ubrr;
	uint32_t _real_baud = real_baud;
	if (baud) {
		// call corrupts the global values, they are cached above.
		calc_ubrr(baud);
		baud = real_baud;
		last_ubrr = _last_ubrr;
		real_baud = _real_baud;
	} else {
		baud = real_baud;
	}
	return baud;
}

int serial_reset(void) {
	if (serial_state != USS_CLOSED) {
		__ENTER_CRITICAL_SECTION__();
		DISABLE_INTR_TR_FIFO_EMPTY();
	}
	serbuf_wr_head = 0;
	serbuf_wr_tail = 0;
	serbuf_wr_used = 0;
	serbuf_wr_free = SERBUF_MAX_LEN;
	serbuf_rd_head = 0;
	serbuf_rd_tail = 0;
	serbuf_rd_used = 0;
	serbuf_rd_free = SERBUF_MAX_LEN;
	if (serial_state != USS_CLOSED) {
		__EXIT_CRITICAL_SECTION__();
	}
	return 0;
}

// Rx Complete
ISR(USART1_RX_vect)
{
	if (serial_state != USS_CLOSED) {
		if (serbuf_rd_used >= SERBUF_MAX_LEN) {
			voidbyte = SER_FIFO; // clear the ISR!
			serbuf_rd_overflow ++;
		} else {
			serbuf_rd[serbuf_rd_head++] = SER_FIFO;
			if (serbuf_rd_head == SERBUF_MAX_LEN) {
				serbuf_rd_head = 0;
			}
			serbuf_rd_used ++;
			serbuf_rd_free --;
		}
	} else {
		// just closed? ISR fired at the same time, just dump the char into void-byte
		// it has to be read to clear the flag
		voidbyte = SER_FIFO;
	}
}

// Tx Complete
ISR(USART1_TX_vect)
{
	DISABLE_INTR_TR_COMPLETE();
}

// DATA Register Empty - UDRE1 bit is set, Tx FIFO can be (re-)filled
ISR(USART1_UDRE_vect)
{
	// Note: if Tx buffer is empty, then disable the UDRE1 flag so this
	//       ISR does not keep re-tripping.
	if (serial_state != USS_CLOSED && serbuf_wr_used > 0) {
		SER_FIFO = serbuf_wr[serbuf_wr_tail++];
		if (serbuf_wr_tail >= SERBUF_MAX_LEN) {
			serbuf_wr_tail = 0;
			//blink_once(1);
		}
		serbuf_wr_used --;
		serbuf_wr_free ++;
	} else {
		DISABLE_INTR_TR_FIFO_EMPTY(); // shutdown the Tx ISR
	}
}


// Normally this should be made thread-safe, but we have no threads...
//		0		nothing sent (tx buffer full)
//		+n		n characters sent (may be a partial transfer). Roll string forward by n and retry later.
int serial_puts(const char * strn, int len) {
	int rc = -1;
	if (serial_state != USS_CLOSED) {
		int max_txfer, iter;
		// quicky turn off Tx interrupts, so we can fiddle with the 
		// Tx buffer without affecting the UART Rx processing
		__ENTER_CRITICAL_SECTION__();
		DISABLE_INTR_TR_FIFO_EMPTY();
		__EXIT_CRITICAL_SECTION__();
		// We can change tx buffer now
		max_txfer = ((uint16_t)len > serbuf_wr_free) 
			? (int)serbuf_wr_free 
			: len;
		if (max_txfer) {
			for (iter = 0; iter < max_txfer ; ++iter) {
				serbuf_wr[serbuf_wr_head++] = strn[iter];
				if (serbuf_wr_head >= SERBUF_MAX_LEN) {
					serbuf_wr_head = 0;
				}
			}
			serbuf_wr_used += max_txfer;
			serbuf_wr_free -= max_txfer;
		}
		if (serbuf_wr_used) {
			// re-enable Tx ISR operations (may immediately fire!)
			ENABLE_INTR_TR_FIFO_EMPTY();
		}
		// All accepted (0) or a partial write (+n NOT sent)
		rc = (int)max_txfer;
	}
	return rc;
}

int serial_putstrn(const char * strn) {
	int rc = -1;
	if (strn) {
		rc = serial_puts(strn, (int)strlen(strn));
	}
	return rc;
}

int serial_peek(void) {
	int rc = -1;
	if (serial_state != USS_CLOSED) {
		// quicky disable all interrupts and get a reading on the
		// Rx buffer status. Not worth the time to just disable Rx
		// interrupts.
		__ENTER_CRITICAL_SECTION__();
		rc = (int)serbuf_rd_used;
		__EXIT_CRITICAL_SECTION__();
	}
	return rc;
}

int serial_gets(char * strn, int maxread) {
	int rc = -1;
	if (serial_state != USS_CLOSED) {
		int max_txfer, iter;
		// quicky turn off Rx interrupts, so we can fiddle with the 
		// Rx buffer without affecting the UART Tx processing
		__ENTER_CRITICAL_SECTION__();
		DISABLE_INTR_RECV_COMPLETE();
		__EXIT_CRITICAL_SECTION__();
		// We can change rx buffer now
		max_txfer = ((uint16_t)maxread > serbuf_rd_used) 
			? (int)serbuf_rd_used 
			: maxread;
		if (max_txfer) {
			for (iter = 0; iter < max_txfer ; ++iter) {
				strn[iter] = serbuf_rd[serbuf_rd_tail++];
				if (serbuf_rd_tail >= SERBUF_MAX_LEN) {
					serbuf_rd_tail = 0;
				}
			}
			serbuf_rd_used -= max_txfer;
			serbuf_rd_free += max_txfer;
		}
		// UNCONDITIONALLY re-enable Rx ISR operations (may immediately fire!)
		ENABLE_INTR_RECV_COMPLETE();
		// nothing to read (0) or n characters copied in.
		rc = (int)max_txfer;
	}
	return rc;
}

int serial_close(void) {
	__ENTER_CRITICAL_SECTION__();
	DISABLE_INTR_RECV_COMPLETE();
	DISABLE_INTR_TR_FIFO_EMPTY();
	SER_CTSR_B = 0;
	serial_state = USS_CLOSED;
	__EXIT_CRITICAL_SECTION__(); // re-enables global interrupt mask, we do not know if others are using it or not.
	return 0;
}

