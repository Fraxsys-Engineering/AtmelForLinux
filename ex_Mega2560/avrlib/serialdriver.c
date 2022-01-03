/****************************************************************************
 * serialdriver.c
 * Simple inteerupt driver serial driver with buffering. Requires additional
 * layer for I/O character stream support (not included in this code).
 * Version 2.0 (Phase C)
 * ---
 * REQUIREMENTS
 * [1.0]     Clocking
 *  [1.1]    Compiler Makefile MUST define F_CPU which is the external
 *          crystal frequency in Hz. For "Mega2560" boards is is normally
 *          16 MHz (16000000UL).
 * PHASES
 * [A]        Simple API, fixed UART, normal speed async only
 * [B]      Convert to driver f-ptrs + dev struct + buffers & bkground ISR
 * [C]      Wrap into a defined set of drivers. Top level API to mount
 *          correct drivers using names, etc. "/dev/uart1"
 *          open: "/dev/uart1,9600,8n1"         (no flow control)
 *                "/dev/uart1,9600,8n1,rtscts"     (H/W flow control)
 * 
 * REQUIRED COMPILE TIME DEFINES
 * 
 *   UART_ENABLE_PORT_n		n:{0,1,2,3}
 *   Enable a minor device, the number equates to the matching physical 
 *   UART peripheral number. Note minor-0 is usually reserved by a 
 *   serial programmer, debugger or other.
 *   (!) More than one define can be setup, this will only enable multiple
 *       concurrent minor devices.
 * 
 * OPTIONAL COMPILE TIME DEFINES
 * 
 *   SERBUF_MAX_LEN n
 *   override the default size of the minor device RX and TX buffers.
 *   The default is 64 bytes for each TX & RX of each enabled driver
 *   so 128 bytes total per minor device.
 *   (!) Smaller Tx buffers really impact the user code when its sending
 *       strings longer than the buffer size and it will have to wait
 *       for room to clear up in the TX queue before adding the remaining
 *       data in a subsequent cd_write() call.
 * 
 *   UART_ENABLE_EMU_1
 *   A special emulation friendly minor number. Use this instead of 
 *   UART_ENABLE_PORT_n when test compiling on Linux target. Outgoing
 *   strings are just printed to stdout0 and input can be manually 
 *   inserted into the minor devices Rx buffer (minor-1) by the 
 *   enabled function minor_emu1_push(const char * buf, int len)
 * 
 ***************************************************************************/


//#include "serialdriver.h"
#include "driver.h"        // the new Driver API
#ifndef UART_ENABLE_EMU_1
 #include <avr/io.h>
 #include <avr/interrupt.h>
#else
 #include <stdio.h>
#endif
#include <string.h>  // string ops.
#include <stdlib.h>  // strtol() etc.


#ifndef SERBUF_MAX_LEN
  #define SERBUF_MAX_LEN 64
#endif


/* Unbelievably annoying that I have to do this, but here are the static
 * Peripheral register locations for UART ports (all MCUs). I think these
 * are all the same for each MCU family. If not then it has to be expanded
 * and switched based on the MCU target being compiled for.
 * Man, this is ANNOYING! System headers should have this info.
 */
#define A_UCSR0A    (sfr8p_t)0xC0
#define A_UCSR0B    (sfr8p_t)0XC1
#define A_UCSR0C    (sfr8p_t)0XC2
#define A_UBRR0L    (sfr8p_t)0xC4
#define A_UBRR0H    (sfr8p_t)0xC5
#define A_UDR0      (sfr8p_t)0XC6
#define A_UCSR1A    (sfr8p_t)0xC8
#define A_UCSR1B    (sfr8p_t)0xC9
#define A_UCSR1C    (sfr8p_t)0xCA
#define A_UBRR1L    (sfr8p_t)0xCC
#define A_UBRR1H    (sfr8p_t)0xCD
#define A_UDR1      (sfr8p_t)0XCE
#define A_UCSR2A    (sfr8p_t)0xD0
#define A_UCSR2B    (sfr8p_t)0xD1
#define A_UCSR2C    (sfr8p_t)0xD2
#define A_UBRR2L    (sfr8p_t)0xD4
#define A_UBRR2H    (sfr8p_t)0xD5
#define A_UDR2      (sfr8p_t)0xD6
#define A_UCSR3A    (sfr8p_t)0x130
#define A_UCSR3B    (sfr8p_t)0x131
#define A_UCSR3C    (sfr8p_t)0x132
#define A_UBRR3L    (sfr8p_t)0x134
#define A_UBRR3H    (sfr8p_t)0x135
#define A_UDR3      (sfr8p_t)0x136


typedef enum serialstate_type {
    USS_CLOSED  = 0,
    USS_IDLE    = 0x01,
    USS_READING = 0x02,
    USS_WRITING = 0x04
    /* end valid values */
} uss_t;

typedef enum serialparity_type {
    SP_NONE = 0,    // 'N' no parity (normal)
    SP_EVEN,        // 'E' even parity
    SP_ODD,         // 'O' odd parity
    /* End of valid enums */
    SP_COUNT
} sparity_t;

typedef enum serialframebits_type {
    SF_5 = 0,        // 5 data bits
    SF_6,            // 6 data bits
    SF_7,            // 7 data bits
    SF_8,            // 8 data bits (normal)
    SF_9,            // 9 data bits (?)
    /* End of valid enums */
    SF_COUNT
} sfbits_t;

typedef enum serialstopbits_type {
    SS_1 = 0,        // 1 stop bit (normal)
    SS_2,            // 2 stop bits 
    /* End of valid enums */
    SS_COUNT
} ssbits_t;

typedef enum serialuartmode_type {
    SM_ASYNC = 0,    // Asynchronous UART - PHASE[A] ONLY ONE SUPPORTED !
    SM_SYNC_TCR,    // Synchronous USART, Tx bit changed on CLK rising edge, Rx sampled on CLK falling edge
    SM_SYNC_TCF,    // Synchronous USART, Tx bit changed on CLK falling edge, Rx sampled on CLK rising edge
    SM_M_SPI,        // Master SPI (MSPIM)
    /* End of valid enums */
    SM_COUNT
} sumode_t;


// Minor Instance Driver Class ----------------------------------------
typedef struct serial_dinst_type {
    int         dev_handle;     // globally assigned device handle, 1+ AKA "file handle"
    int         inst_index;     // The instance number. more than one can be open at the same time, if needed.
    uss_t       state;          // driver state.. used?
    sparity_t   parity;         // UART parity (cached)
    sumode_t    sermode;        // UART peripheral mode (cached)(only SM_ASYNC supported at this time)
    uint8_t     framelen;       // UART databit frame length (cached)
    uint8_t     stopbits;       // UART # stopbits (cached)
    // A simple circular buffer is instantiated on top of this buffer...
    uint16_t    serbuf_wr_head; // where user can write new data
    uint16_t    serbuf_wr_tail; // where driver is pulling data from for Tx serial output
    uint16_t    serbuf_wr_used; // amount used out of total
    uint16_t    serbuf_wr_free; // amount free out of total
    // -
    uint16_t    serbuf_rd_head; // where driver can write new data from Rx serial input
    uint16_t    serbuf_rd_tail; // where user is pulling data from
    uint16_t    serbuf_rd_used; // amount used out of total
    uint16_t    serbuf_rd_free; // amount free out of total
    // -
    uint8_t     voidbyte;       // on errors, read data to this variable to clear flags. this data is ignored.
    uint8_t     _unused;        // align 16-bit
    uint32_t    baud;           // requested baud rate
    uint16_t    last_ubrr;      // last calculated value
    uint32_t    real_baud;      // computed baud rate, not the request one.
    // - Error statistics
    uint32_t    serbuf_rd_overflow;
    // UART Registers (pointers)
    sfr8p_t     udr;
    sfr8p_t     ucsr_a;
    sfr8p_t     ucsr_b;
    sfr8p_t     ucsr_c;
    sfr8p_t     ubrr_h;
    sfr8p_t     ubrr_l;
    // The two intenal circular buffers
    uint8_t     serbuf_wr[SERBUF_MAX_LEN];
    uint8_t     serbuf_rd[SERBUF_MAX_LEN];
} serdinst_t;

// UART Peripheral register map - assigned per minor device
// (each minor device manages its own hardware UART peripheral)
#define SER_FIFO    *(inst->udr)
#define SER_CTSR_A  *(inst->ucsr_a)
#define SER_CTSR_B  *(inst->ucsr_b)
#define SER_CTSR_C  *(inst->ucsr_c)
#define SER_UBRR_H  *(inst->ubrr_h)
#define SER_UBRR_L  *(inst->ubrr_l)
#define SER_UBRR_H_MASK 0x0F  /* upper 4 bits must be written as zero */

// Pre-shifted bits (all bit offsets the same for each uart)
#define MSK_RXCIE   0x80        /* [UCSRnB] Rx Complete INT Enable */
#define MSK_TXCIE   0x40        /* [UCSRnB] Tx Complete INT Enable*/
#define MSK_UDRIE   0x20        /* [UCSRnB] UART Data Register Empty INT Enable */
#define MSK_RXEN    0x10        /* [UCSRnB] RX Enable */
#define MSK_TXEN    0x08        /* [UCSRnB] TX Enable */
#define MSK_RXB8    0x02        /* [UCSRnB] Rx Data Bit 8 (frame=9) */
#define MSK_TXB8    0x01        /* [UCSRnB] Tx Data Bit 8 (frame=9) */

#ifdef UART_ENABLE_EMU_1
 #define F_CPU 8000000L
 void ENABLE_INTR_RECV_COMPLETE()  { printf("*** ENABLE_INTR_RECV_COMPLETE\n"); }
 void DISABLE_INTR_RECV_COMPLETE() { printf("*** DISABLE_INTR_RECV_COMPLETE\n"); }   
 void ENABLE_INTR_TR_FIFO_EMPTY()  { printf("*** ENABLE_INTR_TR_FIFO_EMPTY\n"); }
 void DISABLE_INTR_TR_FIFO_EMPTY() { printf("*** DISABLE_INTR_TR_FIFO_EMPTY\n"); }
 void ENABLE_INTR_TR_COMPLETE()    { printf("*** ENABLE_INTR_TR_COMPLETE\n"); }
 void DISABLE_INTR_TR_COMPLETE()   { printf("*** DISABLE_INTR_TR_COMPLETE\n"); }
 void __ENTER_CRITICAL_SECTION__() { printf("*** __ENTER_CRITICAL_SECTION__ --> cli()\n"); }
 void __EXIT_CRITICAL_SECTION__()  { printf("*** __EXIT_CRITICAL_SECTION__ --> sei()\n"); }
 void sei() { printf("*** sei()\n"); }
#else
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
#endif

// Pre-computes bit tables for frame, stops etc.
static const uint8_t smode_bits[SM_COUNT] = {
    0x00, 0x40, 0x41, 0xC0                // [UCSRnC] - async, sync_tcr, sync_tcf, M.SPI
};
// [n][0] := UCSRnC, Zn0, Zn1
// [n][1] := UCSRnB, Zn2
static const uint8_t frame_bits[SF_COUNT][SS_COUNT] = {
    {0x00, 0x00},    // [UCSRnC],[UCSRnB] 5
    {0x02, 0x00},    // [UCSRnC],[UCSRnB] 6
    {0x04, 0x00},    // [UCSRnC],[UCSRnB] 7
    {0x06, 0x00},    // [UCSRnC],[UCSRnB] 8
    {0x06, 0x04}    // [UCSRnC],[UCSRnB] 9
};
static const uint8_t parity_bits[SP_COUNT] = {
    0x00, 0x20, 0x30                    // [UCSRnC] - none,even,odd
};
static const uint8_t stops_bits[2] = {
    0x00, 0x08                            // [UCSRnC] - 1, 2
};


/* Low Level Driver Routines & ISRs -------------------------------- */

static uint16_t calc_ubrr(serdinst_t * inst, uint32_t baud) {
    // [ F_CPU / (16 x BAUD) ] - 1
    uint32_t lval = F_CPU;
    lval = lval / 16;
    lval = lval / baud;
    inst->last_ubrr = (uint16_t)(lval - 1);
    // F_CPU / [ 16 x (UBRRn + 1) ]
    lval = 16 * (inst->last_ubrr + 1);
    inst->real_baud = (uint32_t)F_CPU / lval;
    return inst->last_ubrr;
}

static int serial_reset(serdinst_t * inst) {
    if (inst->state != USS_CLOSED) {
        __ENTER_CRITICAL_SECTION__();
        DISABLE_INTR_TR_FIFO_EMPTY();
    }
    inst->serbuf_wr_head = 0;
    inst->serbuf_wr_tail = 0;
    inst->serbuf_wr_used = 0;
    inst->serbuf_wr_free = SERBUF_MAX_LEN;
    inst->serbuf_rd_head = 0;
    inst->serbuf_rd_tail = 0;
    inst->serbuf_rd_used = 0;
    inst->serbuf_rd_free = SERBUF_MAX_LEN;
    inst->serbuf_rd_overflow = 0;
    if (inst->state != USS_CLOSED) {
        __EXIT_CRITICAL_SECTION__();
    }
    return 0;
}

static int chk_baud(uint32_t baud) {
    int rc = 0;
    switch(baud) {
    case 9600:
    case 19200:
    case 38400:
    case 57600:
    case 115200:
        rc = 1;
        break;
    default:
        rc = 0;
    }
    return rc;
}

static int chk_parity(const char p) {
    int rc = 0;
    switch (p) {
    case 'N':
    case 'O':
    case 'E':
        rc = 1;
        break;
    default:
        rc = 0;
    }
    return rc;
}

static sparity_t get_parity_type(const char p) {
    sparity_t rc = SP_COUNT;
    switch (p) {
    case 'N':   rc = SP_NONE; break;
    case 'O':   rc = SP_ODD;  break;
    case 'E':   rc = SP_EVEN; break;
    default:    rc = SP_COUNT;
    }
    return rc;
}

static int chk_frametype(int f) {
    int rc = 0;
    switch (f) {
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        rc = 1;
        break;
    default:
        rc = 0;
    }
    return rc;
}

static sfbits_t get_frametype(int f) {
    sfbits_t rc = SF_COUNT;
    switch (f) {
    case 5:  rc = SF_5; break;
    case 6:  rc = SF_6; break;
    case 7:  rc = SF_7; break;
    case 8:  rc = SF_8; break;
    case 9:  rc = SF_9; break;
    default:    rc = SF_COUNT;
    }
    return rc;
}

static int chk_stopbits(int s) {
    int rc = 0;
    switch (s) {
    case 1:
    case 2:
        rc = 1;
        break;
    default:
        rc = 0;
    }
    return rc;
}

static ssbits_t get_stopbits(int s) {
    ssbits_t rc = SS_COUNT;
    switch (s) {
    case 1:  rc = SS_1; break;
    case 2:  rc = SS_2; break;
    default:    rc = SS_COUNT;
    }
    return rc;
}


static int serial_open(serdinst_t * inst, uint32_t baud, sfbits_t frame, ssbits_t stops, sparity_t par) {
    int rc = -1; // assume something went wrong
    if (inst->state == USS_CLOSED && frame < SF_COUNT && stops < SS_COUNT && par < SP_COUNT) {
        calc_ubrr(inst, baud);
        // Register Setup
        SER_UBRR_H = ((uint8_t)(inst->last_ubrr >> 8)) & SER_UBRR_H_MASK;
        SER_UBRR_L = (uint8_t)inst->last_ubrr;
        SER_CTSR_A = 0;
        SER_CTSR_C = smode_bits[SM_ASYNC] | parity_bits[par] | stops_bits[stops] | frame_bits[frame][0];
        SER_CTSR_B = MSK_RXEN | MSK_TXEN | frame_bits[frame][1];
        inst->baud = baud;
        inst->framelen = frame;
        inst->stopbits = stops;
        inst->parity   = par;
        inst->state    = USS_IDLE;
        serial_reset(inst);
        ENABLE_INTR_RECV_COMPLETE(); // Enable receiver only. Tx will be started when it gets something to send.
#if 1
        sei(); // enable global interrupts, if disabled.
#endif
        rc = 0;
    }
    return rc;
}

#if 0
static uint32_t serial_realbaud(serdinst_t * inst, uint32_t baud) {
    uint32_t last_ubrr = inst->last_ubrr;
    uint32_t real_baud = inst->real_baud;
    if (baud) {
        // call corrupts the global values, they are cached above.
        calc_ubrr(inst, baud);
        baud = inst->real_baud;
        inst->last_ubrr = last_ubrr;
        inst->real_baud = real_baud;
    } else {
        baud = inst->real_baud;
    }
    return baud;
}
#endif

// defined in the high level driver layer, at the bottom of this file.
static serdinst_t * s_find_minor_ctx_by_minor_num(int p);

/************** ISR - UART PORT 0 ************************************/
#ifdef UART_ENABLE_PORT_0

// Rx Complete
ISR(USART0_RX_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(0);
    if (inst && inst->state != USS_CLOSED) {
        if (inst->serbuf_rd_used >= SERBUF_MAX_LEN) {
            inst->voidbyte = SER_FIFO; // clear the ISR!
            inst->serbuf_rd_overflow ++;
        } else {
            inst->serbuf_rd[inst->serbuf_rd_head++] = SER_FIFO;
            if (inst->serbuf_rd_head == SERBUF_MAX_LEN) {
                inst->serbuf_rd_head = 0;
            }
            inst->serbuf_rd_used ++;
            inst->serbuf_rd_free --;
        }
    } else {
        // just closed? ISR fired at the same time, just dump the char into void-byte
        // it has to be read to clear the flag
        inst->voidbyte = SER_FIFO;
    }
}

// Tx Complete
ISR(USART0_TX_vect)
{
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(0);
    if (inst)
        DISABLE_INTR_TR_COMPLETE(); // note: this macro expects variable 'inst' to be declared, don't try and optimize the code above!
}

// DATA Register Empty - UDRE1 bit is set, Tx FIFO can be (re-)filled
ISR(USART0_UDRE_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(0);
    // Note: if Tx buffer is empty, then disable the UDRE1 flag so this
    //       ISR does not keep re-tripping.
    if (inst && inst->state != USS_CLOSED && inst->serbuf_wr_used > 0) {
        SER_FIFO = inst->serbuf_wr[inst->serbuf_wr_tail++];
        if (inst->serbuf_wr_tail >= SERBUF_MAX_LEN) {
            inst->serbuf_wr_tail = 0;
        }
        inst->serbuf_wr_used --;
        inst->serbuf_wr_free ++;
    } else {
        DISABLE_INTR_TR_FIFO_EMPTY(); // shutdown the Tx ISR
    }
}

#endif /* UART_ENABLE_PORT_0 */

/************** ISR - UART PORT 1 ************************************/
#ifdef UART_ENABLE_PORT_1

// Rx Complete
ISR(USART1_RX_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(1);
    if (inst && inst->state != USS_CLOSED) {
        if (inst->serbuf_rd_used >= SERBUF_MAX_LEN) {
            inst->voidbyte = SER_FIFO; // clear the ISR!
            inst->serbuf_rd_overflow ++;
        } else {
            inst->serbuf_rd[inst->serbuf_rd_head++] = SER_FIFO;
            if (inst->serbuf_rd_head == SERBUF_MAX_LEN) {
                inst->serbuf_rd_head = 0;
            }
            inst->serbuf_rd_used ++;
            inst->serbuf_rd_free --;
        }
    } else {
        // just closed? ISR fired at the same time, just dump the char into void-byte
        // it has to be read to clear the flag
        inst->voidbyte = SER_FIFO;
    }
}

// Tx Complete
ISR(USART1_TX_vect)
{
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(1);
    if (inst)
        DISABLE_INTR_TR_COMPLETE();
}

// DATA Register Empty - UDRE1 bit is set, Tx FIFO can be (re-)filled
ISR(USART1_UDRE_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(1);
    // Note: if Tx buffer is empty, then disable the UDRE1 flag so this
    //       ISR does not keep re-tripping.
    if (inst && inst->state != USS_CLOSED && inst->serbuf_wr_used > 0) {
        SER_FIFO = inst->serbuf_wr[inst->serbuf_wr_tail++];
        if (inst->serbuf_wr_tail >= SERBUF_MAX_LEN) {
            inst->serbuf_wr_tail = 0;
            //blink_once(1);
        }
        inst->serbuf_wr_used --;
        inst->serbuf_wr_free ++;
    } else {
        DISABLE_INTR_TR_FIFO_EMPTY(); // shutdown the Tx ISR
    }
}

#endif /* UART_ENABLE_PORT_1 */

/************** ISR - UART PORT 2 ************************************/
#ifdef UART_ENABLE_PORT_2

// Rx Complete
ISR(USART2_RX_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(2);
    if (inst && inst->state != USS_CLOSED) {
        if (inst->serbuf_rd_used >= SERBUF_MAX_LEN) {
            inst->voidbyte = SER_FIFO; // clear the ISR!
            inst->serbuf_rd_overflow ++;
        } else {
            inst->serbuf_rd[inst->serbuf_rd_head++] = SER_FIFO;
            if (inst->serbuf_rd_head == SERBUF_MAX_LEN) {
                inst->serbuf_rd_head = 0;
            }
            inst->serbuf_rd_used ++;
            inst->serbuf_rd_free --;
        }
    } else {
        // just closed? ISR fired at the same time, just dump the char into void-byte
        // it has to be read to clear the flag
        inst->voidbyte = SER_FIFO;
    }
}

// Tx Complete
ISR(USART2_TX_vect)
{
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(2);
    if (inst)
        DISABLE_INTR_TR_COMPLETE();
}

// DATA Register Empty - UDRE1 bit is set, Tx FIFO can be (re-)filled
ISR(USART2_UDRE_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(2);
    // Note: if Tx buffer is empty, then disable the UDRE1 flag so this
    //       ISR does not keep re-tripping.
    if (inst && inst->state != USS_CLOSED && inst->serbuf_wr_used > 0) {
        SER_FIFO = inst->serbuf_wr[inst->serbuf_wr_tail++];
        if (inst->serbuf_wr_tail >= SERBUF_MAX_LEN) {
            inst->serbuf_wr_tail = 0;
            //blink_once(1);
        }
        inst->serbuf_wr_used --;
        inst->serbuf_wr_free ++;
    } else {
        DISABLE_INTR_TR_FIFO_EMPTY(); // shutdown the Tx ISR
    }
}

#endif /* UART_ENABLE_PORT_2 */

/************** ISR - UART PORT 3 ************************************/
#ifdef UART_ENABLE_PORT_3

// Rx Complete
ISR(USART3_RX_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(3);
    if (inst && inst->state != USS_CLOSED) {
        if (inst->serbuf_rd_used >= SERBUF_MAX_LEN) {
            inst->voidbyte = SER_FIFO; // clear the ISR!
            inst->serbuf_rd_overflow ++;
        } else {
            inst->serbuf_rd[inst->serbuf_rd_head++] = SER_FIFO;
            if (inst->serbuf_rd_head == SERBUF_MAX_LEN) {
                inst->serbuf_rd_head = 0;
            }
            inst->serbuf_rd_used ++;
            inst->serbuf_rd_free --;
        }
    } else {
        // just closed? ISR fired at the same time, just dump the char into void-byte
        // it has to be read to clear the flag
        inst->voidbyte = SER_FIFO;
    }
}

// Tx Complete
ISR(USART3_TX_vect)
{
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(3);
    if (inst)
        DISABLE_INTR_TR_COMPLETE();
}

// DATA Register Empty - UDRE1 bit is set, Tx FIFO can be (re-)filled
ISR(USART3_UDRE_vect)
{
    // cannot pass data in, ISR must get the instance from the root driver.
    // if the port is closed then NULL is returned!
    serdinst_t * inst = s_find_minor_ctx_by_minor_num(3);
    // Note: if Tx buffer is empty, then disable the UDRE1 flag so this
    //       ISR does not keep re-tripping.
    if (inst && inst->state != USS_CLOSED && inst->serbuf_wr_used > 0) {
        SER_FIFO = inst->serbuf_wr[inst->serbuf_wr_tail++];
        if (inst->serbuf_wr_tail >= SERBUF_MAX_LEN) {
            inst->serbuf_wr_tail = 0;
            //blink_once(1);
        }
        inst->serbuf_wr_used --;
        inst->serbuf_wr_free ++;
    } else {
        DISABLE_INTR_TR_FIFO_EMPTY(); // shutdown the Tx ISR
    }
}

#endif /* UART_ENABLE_PORT_3 */

/************** END OF ALL ISRs **************************************/



// Normally this should be made thread-safe, but we have no threads...
//        0        nothing sent (tx buffer full)
//        +n        n characters sent (may be a partial transfer). Roll string forward by n and retry later.
static int serial_puts(serdinst_t * inst, const char * strn, int len) {
    int rc = -1;
    if (inst->state != USS_CLOSED) {
        int max_txfer, iter;
        // quicky turn off Tx interrupts, so we can fiddle with the 
        // Tx buffer without affecting the UART Rx processing
        __ENTER_CRITICAL_SECTION__();
        DISABLE_INTR_TR_FIFO_EMPTY();
        __EXIT_CRITICAL_SECTION__();
        // We can change tx buffer now
        max_txfer = ((uint16_t)len > inst->serbuf_wr_free) 
            ? (int)inst->serbuf_wr_free 
            : len;
        if (max_txfer) {
            for (iter = 0; iter < max_txfer ; ++iter) {
                inst->serbuf_wr[inst->serbuf_wr_head++] = strn[iter];
                if (inst->serbuf_wr_head >= SERBUF_MAX_LEN) {
                    inst->serbuf_wr_head = 0;
                }
            }
            inst->serbuf_wr_used += max_txfer;
            inst->serbuf_wr_free -= max_txfer;
        }
        if (inst->serbuf_wr_used) {
            // re-enable Tx ISR operations (may immediately fire!)
            ENABLE_INTR_TR_FIFO_EMPTY();
        }
        // All accepted (0) or a partial write (+n NOT sent)
        rc = (int)max_txfer;
    }
    return rc;
}

#if 0
static int serial_putstrn(serdinst_t * inst, const char * strn) {
    int rc = -1;
    if (strn) {
        rc = serial_puts(inst, strn, (int)strlen(strn));
    }
    return rc;
}
#endif

static int serial_rd_peek(serdinst_t * inst) {
    int rc = -1;
    if (inst->state != USS_CLOSED) {
        // quicky disable all interrupts and get a reading on the
        // Rx buffer status. Not worth the time to just disable Rx
        // interrupts.
        __ENTER_CRITICAL_SECTION__();
        rc = (int)inst->serbuf_rd_used;
        __EXIT_CRITICAL_SECTION__();
    }
    return rc;
}

static int serial_wr_peek(serdinst_t * inst) {
    int rc = -1;
    if (inst->state != USS_CLOSED) {
        // quicky disable all interrupts and get a reading on the
        // Rx buffer status. Not worth the time to just disable Rx
        // interrupts.
        __ENTER_CRITICAL_SECTION__();
        rc = (int)inst->serbuf_wr_free;
        __EXIT_CRITICAL_SECTION__();
    }
    return rc;
}

static int serial_gets(serdinst_t * inst, char * strn, int maxread) {
    int rc = -1;
    if (inst->state != USS_CLOSED) {
        int max_txfer, iter;
        // quicky turn off Rx interrupts, so we can fiddle with the 
        // Rx buffer without affecting the UART Tx processing
        __ENTER_CRITICAL_SECTION__();
        DISABLE_INTR_RECV_COMPLETE();
        __EXIT_CRITICAL_SECTION__();
        // We can change rx buffer now
        max_txfer = ((uint16_t)maxread > inst->serbuf_rd_used) 
            ? (int)inst->serbuf_rd_used 
            : maxread;
        if (max_txfer) {
            for (iter = 0; iter < max_txfer ; ++iter) {
                strn[iter] = inst->serbuf_rd[inst->serbuf_rd_tail++];
                if (inst->serbuf_rd_tail >= SERBUF_MAX_LEN) {
                    inst->serbuf_rd_tail = 0;
                }
            }
            inst->serbuf_rd_used -= max_txfer;
            inst->serbuf_rd_free += max_txfer;
        }
        // UNCONDITIONALLY re-enable Rx ISR operations (may immediately fire!)
        ENABLE_INTR_RECV_COMPLETE();
        // nothing to read (0) or n characters copied in.
        rc = (int)max_txfer;
    }
    return rc;
}

static int serial_close(serdinst_t * inst) {
    __ENTER_CRITICAL_SECTION__();
    DISABLE_INTR_RECV_COMPLETE();
    DISABLE_INTR_TR_FIFO_EMPTY();
    SER_CTSR_B = 0;
    inst->state = USS_CLOSED;
    __EXIT_CRITICAL_SECTION__(); // re-enables global interrupt mask, we do not know if others are using it or not.
    return 0;
}


/******************** DRIVER INSTANCE METHODS ************************/


#define DEV_ISOPEN(dev)    (dev->dev_handle > 0)
#define DEV_ISCLOSED(dev)  (dev->dev_handle == 0)


#ifdef UART_ENABLE_PORT_0
static serdinst_t uart_minor0 = {
    .dev_handle     = 0,    // invalid file handle, signals this driver is not active.
    .inst_index     = 0,    // minor-0
    .state          = USS_CLOSED,
    .serbuf_wr_head = 0,
    .serbuf_wr_tail = 0,
    .serbuf_wr_used = 0,
    .serbuf_wr_free = 0,
    .serbuf_rd_head = 0,
    .serbuf_rd_tail = 0,
    .serbuf_rd_used = 0,
    .serbuf_rd_free = 0,
    .udr            = A_UDR0,
    .ucsr_a         = A_UCSR0A,
    .ucsr_b         = A_UCSR0B,
    .ucsr_c         = A_UCSR0C,
    .ubrr_h         = A_UBRR0H,
    .ubrr_l         = A_UBRR0L
};
static serdinst_t * const p_uart_minor0 = &uart_minor0;
#else
static serdinst_t * const p_uart_minor0 = NULL;
#endif /* UART_ENABLE_PORT_0 */

#ifdef UART_ENABLE_PORT_1
static serdinst_t uart_minor1 = {
    .dev_handle = -1,   // invalid file handle, signals this driver is not active.
    .inst_index = 1,    // minor-1
    .state = USS_CLOSED,
    .serbuf_wr_head = 0,
    .serbuf_wr_tail = 0,
    .serbuf_wr_used = 0,
    .serbuf_wr_free = 0,
    .serbuf_rd_head = 0,
    .serbuf_rd_tail = 0,
    .serbuf_rd_used = 0,
    .serbuf_rd_free = 0,
    .udr            = A_UDR1,
    .ucsr_a         = A_UCSR1A,
    .ucsr_b         = A_UCSR1B,
    .ucsr_c         = A_UCSR1C,
    .ubrr_h         = A_UBRR1H,
    .ubrr_l         = A_UBRR1L
};
static serdinst_t * const p_uart_minor1 = &uart_minor1;
#else
 #ifndef UART_ENABLE_EMU_1
static serdinst_t * const p_uart_minor1 = NULL;
 #endif
#endif /* UART_ENABLE_PORT_1 */

#ifdef UART_ENABLE_PORT_2
static serdinst_t uart_minor2 = {
    .dev_handle = -1,   // invalid file handle, signals this driver is not active.
    .inst_index = 2,    // minor-2
    .state = USS_CLOSED,
    .serbuf_wr_head = 0,
    .serbuf_wr_tail = 0,
    .serbuf_wr_used = 0,
    .serbuf_wr_free = 0,
    .serbuf_rd_head = 0,
    .serbuf_rd_tail = 0,
    .serbuf_rd_used = 0,
    .serbuf_rd_free = 0,
    .udr            = A_UDR2,
    .ucsr_a         = A_UCSR2A,
    .ucsr_b         = A_UCSR2B,
    .ucsr_c         = A_UCSR2C,
    .ubrr_h         = A_UBRR2H,
    .ubrr_l         = A_UBRR2L
};
static serdinst_t * const p_uart_minor2 = &uart_minor2;
#else
static serdinst_t * const p_uart_minor2 = NULL;
#endif /* UART_ENABLE_PORT_2 */

#ifdef UART_ENABLE_PORT_3
static serdinst_t uart_minor3 = {
    .dev_handle = -1,   // invalid file handle, signals this driver is not active.
    .inst_index = 3,    // minor-3
    .state = USS_CLOSED,
    .serbuf_wr_head = 0,
    .serbuf_wr_tail = 0,
    .serbuf_wr_used = 0,
    .serbuf_wr_free = 0,
    .serbuf_rd_head = 0,
    .serbuf_rd_tail = 0,
    .serbuf_rd_used = 0,
    .serbuf_rd_free = 0,
    .udr            = A_UDR3,
    .ucsr_a         = A_UCSR3A,
    .ucsr_b         = A_UCSR3B,
    .ucsr_c         = A_UCSR3C,
    .ubrr_h         = A_UBRR3H,
    .ubrr_l         = A_UBRR3L
};
static serdinst_t * const p_uart_minor3 = &uart_minor3;
#else
static serdinst_t * const p_uart_minor3 = NULL;
#endif /* UART_ENABLE_PORT_3 */

#ifdef UART_ENABLE_EMU_1
 #if defined(UART_ENABLE_PORT_0) || defined(UART_ENABLE_PORT_1) || defined(UART_ENABLE_PORT_2) || defined(UART_ENABLE_PORT_3)
  #error "AVR Target UARTs cannot be enabled while emulating minor device 1!"
 #endif 

static uint8_t dummyreg_udr1; 
static uint8_t dummyreg_ucsr1a; 
static uint8_t dummyreg_ucsr1b; 
static uint8_t dummyreg_ucsr1c; 
static uint8_t dummyreg_ubrr1h; 
static uint8_t dummyreg_ubrr1l;

static serdinst_t uart_emu_minor1 = {
    .dev_handle = -1,   // invalid file handle, signals this driver is not active.
    .inst_index = 1,    // minor-1
    .state = USS_CLOSED,
    .serbuf_wr_head = 0,
    .serbuf_wr_tail = 0,
    .serbuf_wr_used = 0,
    .serbuf_wr_free = 0,
    .serbuf_rd_head = 0,
    .serbuf_rd_tail = 0,
    .serbuf_rd_used = 0,
    .serbuf_rd_free = 0,
    .udr            = (sfr8p_t)(&dummyreg_udr1),
    .ucsr_a         = (sfr8p_t)(&dummyreg_ucsr1a),
    .ucsr_b         = (sfr8p_t)(&dummyreg_ucsr1b),
    .ucsr_c         = (sfr8p_t)(&dummyreg_ucsr1c),
    .ubrr_h         = (sfr8p_t)(&dummyreg_ubrr1h),
    .ubrr_l         = (sfr8p_t)(&dummyreg_ubrr1l)
};
static serdinst_t * const p_uart_minor1 = &uart_emu_minor1;

uint8_t uart_emu1_rd_dummyreg_udr1(void)   { return dummyreg_udr1; }
uint8_t uart_emu1_rd_dummyreg_ucsr1a(void) { return dummyreg_ucsr1a; }
uint8_t uart_emu1_rd_dummyreg_ucsr1b(void) { return dummyreg_ucsr1b; }
uint8_t uart_emu1_rd_dummyreg_ucsr1c(void) { return dummyreg_ucsr1c; }
uint8_t uart_emu1_rd_dummyreg_ubrr1h(void) { return dummyreg_ubrr1h; }
uint8_t uart_emu1_rd_dummyreg_ubrr1l(void) { return dummyreg_ubrr1l; }
// pull pending TX data from the transmit buffer, instead of an ISR doing it
int uart_emu1_get_tx(char * strn, int maxread) {
    int rc = -1;
    serdinst_t * inst = (serdinst_t *)p_uart_minor1;
    if (inst->state != USS_CLOSED) {
        int max_txfer, iter;
        max_txfer = ((uint16_t)maxread > inst->serbuf_wr_used) 
            ? (int)inst->serbuf_wr_used 
            : maxread;
        if (max_txfer) {
            for (iter = 0; iter < max_txfer ; ++iter) {
                strn[iter] = inst->serbuf_wr[inst->serbuf_wr_tail++];
                if (inst->serbuf_wr_tail >= SERBUF_MAX_LEN) {
                    inst->serbuf_wr_tail = 0;
                }
            }
            inst->serbuf_wr_used -= max_txfer;
            inst->serbuf_wr_free += max_txfer;
        }
        rc = (int)max_txfer;
    }
    return rc;
}
// put data into the RX buffer so that it can be read by cd_read() call
int uart_emu1_put_rx(const char * strn, int len) {
    int rc = -1;
    serdinst_t * inst = (serdinst_t *)p_uart_minor1;
    if (inst->state != USS_CLOSED) {
        int max_txfer, iter;
        max_txfer = ((uint16_t)len > inst->serbuf_rd_free) 
            ? (int)inst->serbuf_rd_free 
            : len;
        if (max_txfer) {
            for (iter = 0; iter < max_txfer ; ++iter) {
                inst->serbuf_rd[inst->serbuf_rd_head++] = strn[iter];
                if (inst->serbuf_rd_head >= SERBUF_MAX_LEN) {
                    inst->serbuf_rd_head = 0;
                }
            }
            inst->serbuf_rd_used += max_txfer;
            inst->serbuf_rd_free -= max_txfer;
        }
        rc = (int)max_txfer;
    }
    return rc;
}
#endif

/* Driver Private Data - Global Context ------------------------------*/

#define UART_COUNT 4   /* always the same, should not have to change this! */

typedef struct uart_context_type {
    // MINOR DEVICE INFO:
    //   UART Minor devices (if any) have a non-null pointer address
    //   filled in at the appropriate index, where the index matches 
    //   the minor number as well.
    serdinst_t * instlist[UART_COUNT];
} uart_ctx_t;

static uart_ctx_t _uart_ctx = { 0 };

static const char driver_name_prefix[] = "/dev/uart/";


static serdinst_t * s_find_minor_ctx_by_minor_num( int minor ) {
	// simple indexing lookup... minor number *is* the index!
    return (minor >= 0 && minor < UART_COUNT) ? _uart_ctx.instlist[minor] : NULL;
}

static serdinst_t * s_find_minor_ctx_by_filehandle( int hndl ) {
	// find an entry with a matching file handle (device handle)
	serdinst_t * ctx = NULL;
	int iter;
	for ( iter = 0 ; iter < UART_COUNT ; ++iter ) {
		if ( _uart_ctx.instlist[iter] && _uart_ctx.instlist[iter]->dev_handle == hndl) {
			ctx = _uart_ctx.instlist[iter];
			break;
		}
	}
	return ctx;
}

#if 0
static int s_find_minor_ctxidx_by_filehandle( int hndl ) {
	// find an entry with a matching file handle (device handle)
	int cidx = -1;
	int iter;
	for ( iter = 0 ; iter < UART_COUNT ; ++iter ) {
		if ( _uart_ctx.instlist[iter] && _uart_ctx.instlist[iter]->dev_handle == hndl) {
			cidx = iter;
			break;
		}
	}
	return cidx;
}
#endif

static void s_reset_driver_instance( int minor, int do_full_init ) {
    if(minor >= 0 && minor < UART_COUNT) {
        serdinst_t * pc = _uart_ctx.instlist[minor];
        if (pc) {
            if (do_full_init) {
                // only when performing initial device initializations... NO OTHER TIME!
                pc->dev_handle = 0;
                pc->state = USS_CLOSED;
            }
            serial_reset(pc); // call a low-level method to properly setup FIFOs.
        }
    }
}

//static void s_reset_driver_device( int hndl ) {
//    s_reset_driver_instance( s_find_minor_ctxidx_by_filehandle(hndl) );
//}

// f_init()
static void uart_init(void) {
	int iter;
	for ( iter = 0 ; iter < UART_COUNT ; ++iter ) {
        s_reset_driver_instance(iter,1); // reset all minor instances, if they exist.
    }
}

// f_open()
static int uart_open(const char * name, const int mode) {
    int rc = DEV_FAIL;
    if (name && strlen(name) > strlen(driver_name_prefix)) {
        uint32_t baud  = 9600;
        sfbits_t frame = SF_8;
        ssbits_t stops = SS_1;
        sparity_t par  = SP_NONE;
        char * ep      = NULL;
        char * sp      = (char *)name + strlen(driver_name_prefix);
        int minor      = (int)strtol(sp,&ep,10);
        serdinst_t * devctx = s_find_minor_ctx_by_minor_num(minor);

        if (devctx && DEV_ISCLOSED(devctx)) {
            int n;
            // parse further optional fields: ,baud,bits,parity,stops
            if (ep && *ep == ',') {
                // [int] baud
                sp = ep+1;  ep = NULL;
                n = (int)strtol(sp,&ep,10);
                if (chk_baud(n)) {
                    baud = n;
                } else {
                    return -1; // invalid baud rate
                }
                if (ep && *ep == ',') {
                    // [int] frame bit count
                    sp = ep+1;  ep = NULL;
                    n = (int)strtol(sp,&ep,10);
                    if (chk_frametype(n)) {
                        frame = get_frametype(n);
                    } else {
                        return -1; // invalid # bits in frame requested 
                    }
                    if (ep && *ep == ',') {
                        // [char] parity:= {N,E,O} N[one], E[ven], O[dd]
                        ep ++;
                        if (*ep != '\0' && chk_parity(*ep)) {
                            par = get_parity_type(*ep);
                        } else {
                            return -1; // invalid parity code
                        }
                        ep ++;
                        if (*ep == ',') {
                            // [int] stop bits
                            sp = ep+1;  ep = NULL;
                            n = (int)strtol(sp,&ep,10);
                            if (chk_stopbits(n)) {
                                stops = get_stopbits(n);
                            } else {
                                return -1; // invalid stop bits count
                            }
                        }
                    }                    
                }
            }
            rc = serial_open(devctx, baud, frame, stops, par);
            if (rc == DEV_SUCCESS) {
                devctx->dev_handle = Driver_getHandle();
                rc = devctx->dev_handle;
            }
        }
    }
    return rc;
}

// f_close()
static int uart_close(int hndl) {
    int rc = -1;
    serdinst_t * devctx = s_find_minor_ctx_by_filehandle(hndl);
    if (devctx && DEV_ISOPEN(devctx)) {
        serial_close(devctx);
        devctx->dev_handle = 0; // invalidate the file handle that was using it
        rc = 0;
    }
    return rc;
}
    
// f_read()
static int uart_read(int hndl, char * buffer, int len) {
    int rc = -1;
    serdinst_t * devctx = s_find_minor_ctx_by_filehandle(hndl);
    if (devctx && DEV_ISOPEN(devctx)) {
        rc = serial_gets(devctx, buffer, len);
    }
    return rc;
}

// f_write()
static int uart_write(int hndl, const char * buffer, int len) {
    int rc = -1;
    serdinst_t * devctx = s_find_minor_ctx_by_filehandle(hndl);
    if (devctx && DEV_ISOPEN(devctx)) {
        rc = serial_puts(devctx, buffer, len);
    }
    return rc;
}

// f_reset()
static int uart_reset(int hndl) {
    int rc = -1;
    serdinst_t * devctx = s_find_minor_ctx_by_filehandle(hndl);
    if (devctx && DEV_ISOPEN(devctx)) {
        s_reset_driver_instance( devctx->inst_index, 0);
        rc = 0;
    }
    return rc;
}

// f_ioctl()
static int uart_ioctl(int hndl, int cmd, int * val) {
    int rc = -1;
    serdinst_t * devctx = s_find_minor_ctx_by_filehandle(hndl);
    if (devctx && DEV_ISOPEN(devctx) && val) {
		switch (cmd) {
        case CMD_RX_PEEK:   // (cmd) --> (int)n   returns # bytes waiting in Rx buffer
            *val = serial_rd_peek(devctx);
			rc = 0;
			break;
        
        case CMD_TX_PEEK:   // (cmd) --> (int)n   returns # bytes of space available in Tx buffer
			*val = serial_wr_peek(devctx);
			rc = 0;
			break;
		
        case CMD_RD_BAUD:
            *val = (int)(devctx->real_baud);
            rc = 0;
            break;
            
		default:
			rc = -1;
		}
    }
    return rc;
}


/******************** GLOBAL DRIVER AND INSTANCES ********************/

/* Driver OBJECT (Parent) --------------------------------------------*/

static hdriver_t _uart_inst = {
    uart_init,          // init     (global)
    uart_open,          // open     (global)
    uart_close,         // close    (ctx)
    uart_read,          // read     (ctx)
    uart_write,         // write    (ctx)
    uart_reset,         // reset    (ctx)
    uart_ioctl,         // ioctl    (ctx)
    (void *)&_uart_ctx  // driver-class opaque data     (global)
};

/* Driver Registration - Called by main() ----------------------------*/

int uart_Register(void) {
    _uart_ctx.instlist[0] = p_uart_minor0;
    _uart_ctx.instlist[1] = p_uart_minor1;
    _uart_ctx.instlist[2] = p_uart_minor2;
    _uart_ctx.instlist[3] = p_uart_minor3;
    return Driver_registerClass(driver_name_prefix, &_uart_inst);
}
