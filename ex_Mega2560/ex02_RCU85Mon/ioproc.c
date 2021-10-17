/****************************************************************************
 * ioproc.c
 * Keyboard Scan and 7-Segment LED Dispay Updater
 * Version 1.0
 * ---
 * SHORT - Design as the glue logic for the RCY-85 Programming keyboard. 
 *         This will eventually also connect into the 8085A Memory bus and
 *         act as a monitor to program and read RAM Data.
 * ---
 * TODO PHASES
 * [B] Add ability to HOLD 8085 CPU and directly access the memory space.
 *     ref. Atmel Mega2560 EMI interface or GPIO if EMI is too fast.
 * [C] Serial interface and Monitor CLI. Support for file downloading
 *     X,Y,Z-Modem? into a local RAM buffer then off-loading into RCU-85
 *     Memory.
 ***************************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>  // uint8_t etc.

// Physical Description of the RCU-85 Keyboard/LED controller
// Target: ATMega2560 (Arduino Mega2560)

// Display: ICM7218A
//
// 7-Segment Display Configuration
// +--+--+--+--+  +--+--+
// |  |  |  |  |  |  |  |
// |  |  |  |  |  |  |  |
// +--+--+--+--+  +--+--+
//  N3 N2 N1 N0    n1 n0
// +-----+-----+  +-----+
//   MSB   LSB     Byte
// +-----------+  +-----+
//   Address       Data
//
// 7-Seg. Display Assignment
//  D1 - N3
//  D2 - N2
//  D3 - N1
//  D4 - N0
//  D5 - n1
//  D6 - n0
//  D7 - N/U (future)
//  D8 - N/U (future)
//
// ICM7218A Pinning
// IO0    - A0  PORTF0
// IO1    - A1  PORTF1
// IO2    - A2  PORTF2
// IO3    - A3  PORTF3
// IO4    - A4  PORTF4
// IO5    - A5  PORTF5
// IO6    - A6  PORTF6
// IO7    - A7  PORTF7
// /WRITE - A8  PORTK0
// MODE   - A9  PORTK1

// Ports
#define DISP_CRTL_IO    PORTF
#define DISP_CRTL_WR    PORTK
#define DISP_CRTL_MD    PORTK
// Port-DDR
#define DISP_DIR__IO    DDRF
#define DISP_DIR__WR    DDRK
#define DISP_DIR__MD    DDRK
// Port-Bit (Pin)
#define DISP_PIN_IO0    PF0
#define DISP_PIN_IO1    PF1
#define DISP_PIN_IO2    PF2
#define DISP_PIN_IO3    PF3
#define DISP_PIN_IO4    PF4
#define DISP_PIN_IO5    PF5
#define DISP_PIN_IO6    PF6
#define DISP_PIN_IO7    PF7
#define DISP_PIN__WR    PK0
#define DISP_PIN__MD    PK1
// Port BitMask
#define DISP_DMSK_IO    0xff
#define DISP_DMSK_WR    (1<<DISP_PIN__WR)
#define DISP_DMSK_MD    (1<<DISP_PIN__MD)

// ICM7218A Programming ---------------
#define DISP_PIN_BSEL      DISP_PIN_IO3  /* Bank Select      */
#define DISP_PIN_SHTDN     DISP_PIN_IO4  /* Shutdown         */
#define DISP_PIN_DEC_NODEC DISP_PIN_IO5  /* Decode/No-Decode */
#define DISP_PIN_HEX_CODEB DISP_PIN_IO6  /* HEX/CODE-B       */
#define DISP_PIN_DAT_START DISP_PIN_IO7  /* Start Update     */
#define DISP_BANK_A        1
#define DISP_BANK_B        0
#define DISP_RUN           1
#define DISP_SHTDN         0
#define DISP_NO_DECODE     1
#define DISP_DECODE        0
#define DISP_DEC_HEX       1
#define DISP_DEC_CODEB     0
#define DISP_DAT_START     1
#define DISP_MODE_CTRL     1
#define DISP_MODE_DATA     0
#define DISP_WRITE         0  /* active low */
// Predefined CONTROL setups
#define DISP_CTRL_INIT         (DISP_BANK_A << DISP_PIN_BSEL) | (DISP_RUN << DISP_PIN_SHTDN) | (DISP_DEC_HEX << DISP_PIN_HEX_CODEB)
#define DISP_CTRL_START_UPDATE (DISP_BANK_A << DISP_PIN_BSEL) | (DISP_RUN << DISP_PIN_SHTDN) | (DISP_DEC_HEX << DISP_PIN_HEX_CODEB) | (DISP_DAT_START << DISP_PIN_DAT_START)
// Hex Data input on: IO[3..0] !
// Display Databus Timing
#define DISP_TIM_PRE_US     1
#define DISP_TIM_HOLD_US    1
#define DISP_TIM_POST_US    1


// Keyboard:  Parallel-To-Serial (Chained 74LS165)
// Bit-Width: 24 (Final: 32)
//  BIT USE         BIT USE         BIT USE
//  --- ----------- --- ----------- --- -----------
//  23  A15         15  A7           7  D7
//  22  A14         14  A6           6  D6
//  21  A13         13  A5           5  D5
//  20  A12         12  A4           4  D4
//  19  A11         11  A3           3  D3
//  18  A10         10  A2           2  D2
//  17  A9           9  A1           1  D1
//  16  A8           8  A0           0  D0
// ------------------------------------------------
// Shift Order: Bit-0 First, 24 shifts total
// Load: Active Low, Latch on Rising Edge
//
// 74LS165 Pinning:
//  /LOAD           A12     PORTK4
//  CLOCK           A13     PORTK5
//  SerDataOut      A14     PORTK6

#define KYBD_CRTL       PORTK
#define KYBD_CRTL_LOAD  PORTK
#define KYBD_CRTL_CLK   PORTK
#define KYBD_CRTL_DIN   PINK
#define KYBD_DDR        DDRK
#define KYBD_PIN_LOAD   PK4
#define KYBD_PIN_CLK    PK5
#define KYBD_PIN_DIN    PINK6
#define KYBD_DIN_MASK   (1 << KYBD_PIN_DIN)

#define KYBD_SHIFTER_COUNT  3
#define KYBS_STRUCTBUS_SZ   4  /* to accomodate 4 byte int bit-mask, extra byte will bbe used in future. */
#define KYBD_LOAD_DLY_US    1
#define KYBD_CLK_PD_US      1


//#define LED_PIN_DDR DDRB
//#define LED_PIN_BIT DDB7
//#define LED_PIN_PRT PORTB




typedef struct keybd_type {
    union ds_type {
        // raw bytes. right-most shifter gets loaded into 
        // byt[0] then next shifter gets into the next byte.
        // bytes shift RIGHT while b7 is updated (ITER8: in->b7; shift-right)
        uint8_t byt[KYBS_STRUCTBUS_SZ];
        struct dat_type {
            uint8_t data;
            uint8_t addr_lo;
            uint8_t addr_hi;
            uint8_t _unused;
        } dat;
        struct bit_type {
            int d7  : 1;
            int d6  : 1;
            int d5  : 1;
            int d4  : 1;
            int d3  : 1;
            int d2  : 1;
            int d1  : 1;
            int d0  : 1;
            int a7  : 1;
            int a6  : 1;
            int a5  : 1;
            int a4  : 1;
            int a3  : 1;
            int a2  : 1;
            int a1  : 1;
            int a0  : 1;
            int a15 : 1;
            int a14 : 1;
            int a13 : 1;
            int a12 : 1;
            int a11 : 1;
            int a10 : 1;
            int a9  : 1;
            int a8  : 1;
            int _uu : 8;
        } bit;
    } ds;
} kybd_t;


#define PRTBIT_OUT(D,B)  D |= (1 << B);
#define PRTBIT__IN(D,B)  D &= (~(1 << B));
#define PRTBIT_SET(P,B)  P |= (1 << B);
#define PRTBIT_CLR(P,B)  P &= (~(1<< B));
#define PRTBIT_TST(P,M)  (P & M)


// call FIRST. Resets all affected ports to INPUT and sets output port data to 0
void port_init(void) {
    KYBD_CRTL = 0;  // any outputs set low (kybd, disp-ctrl)
    KYBD_DDR = 0;   // set all input (kybd, disp-ctrl)
    DISP_CRTL_IO = 0; // the 8 IO bits for ICM7218A
    DISP_DIR__IO = 0;
}

// call to setup keyboard scanning
void kybd_init(void) {
    // LOAD: OUT, high
    // CLK: OUT, low
    // DIN: IN Hi-z
    PRTBIT_OUT(KYBD_DDR, KYBD_PIN_LOAD);
    PRTBIT_OUT(KYBD_DDR, KYBD_PIN_CLK);
    PRTBIT_SET(KYBD_CRTL_LOAD, KYBD_PIN_LOAD);
}

// Write a byte to the display
void disp_write(uint8_t byt, uint8_t mode) {
    DISP_CRTL_IO = byt;
    if (mode == DISP_MODE_CTRL) {
        // write a control byte (this may start a transfer)
        PRTBIT_SET(DISP_CRTL_MD, DISP_PIN__MD);
    } else {
        // write a "data" byte
        PRTBIT_CLR(DISP_CRTL_MD, DISP_PIN__MD);
    }
    _delay_us(DISP_TIM_PRE_US);
    PRTBIT_CLR(DISP_CRTL_WR, DISP_PIN__WR);
    _delay_us(DISP_TIM_HOLD_US);
    PRTBIT_SET(DISP_CRTL_WR, DISP_PIN__WR);
    _delay_us(DISP_TIM_POST_US);
}

// call to setup display control
void disp_init(void) {
    uint8_t disp = DISP_CTRL_INIT;
    PRTBIT_SET(DISP_CRTL_WR, DISP_PIN__WR); // set high before changing to output
    PRTBIT_OUT(DISP_DIR__WR, DISP_PIN__WR);
    PRTBIT_OUT(DISP_DIR__MD, DISP_PIN__MD);
    DISP_DIR__IO = DISP_DMSK_IO; // all of PORTF is output
    // setup the ICM7218A for HEX output
    disp_write(disp, DISP_MODE_CTRL);
}

void kybd_clk(void) {
	PRTBIT_SET(KYBD_CRTL_CLK, KYBD_PIN_CLK);
	_delay_us(KYBD_CLK_PD_US);
	PRTBIT_CLR(KYBD_CRTL_CLK, KYBD_PIN_CLK);
	_delay_us(KYBD_CLK_PD_US);
}

// call to invoke one key-scan. Blocks until done.
void kybd_scan(kybd_t * kdata) {
    uint8_t d,p,b;
    // Cycle /LOAD to scan the keys into FF's
    PRTBIT_CLR(KYBD_CRTL_LOAD, KYBD_PIN_LOAD);
    _delay_us(KYBD_LOAD_DLY_US);
    PRTBIT_SET(KYBD_CRTL_LOAD, KYBD_PIN_LOAD);
    _delay_us(KYBD_LOAD_DLY_US);
    for (p = 0 ; p < KYBD_SHIFTER_COUNT ; ++p) {
        d = 0;
        // shift 7 times!
        for (b = 0 ; b < 7 ; ++b) {
            if ( PRTBIT_TST(KYBD_CRTL_DIN, KYBD_DIN_MASK) ) {
                d |= 0x80; // DIN ~ '1'
            }
            d = d >> 1;
            kybd_clk(); // 1 .. 7
        }
        // 8th bit test for bit-7
        if ( PRTBIT_TST(KYBD_CRTL_DIN, KYBD_DIN_MASK) ) {
            d |= 0x80;
        }
        kdata->ds.byt[p] = d;
        kybd_clk(); // 8
    }
}

// call to refresh the LED 7-Segment Display
void disp_update(kybd_t * disp) {
    uint8_t dd = DISP_CTRL_START_UPDATE;
    disp_write(dd, DISP_MODE_CTRL);
    // A[15..12]
    dd = (disp->ds.dat.addr_hi >> 4) & 0x0f;
    disp_write(dd, DISP_MODE_DATA);
    // A[11..8]
    dd = disp->ds.dat.addr_hi & 0x0f;
    disp_write(dd, DISP_MODE_DATA);
    // A[7..4]
    dd = (disp->ds.dat.addr_lo >> 4) & 0x0f;
    disp_write(dd, DISP_MODE_DATA);
    // A[3..0]
    dd = disp->ds.dat.addr_lo & 0x0f;
    disp_write(dd, DISP_MODE_DATA);
    // D[7..4]
    dd = (disp->ds.dat.data >> 4) & 0x0f;
    disp_write(dd, DISP_MODE_DATA);
    // D[3..0]
    dd = disp->ds.dat.data & 0x0f;
    disp_write(dd, DISP_MODE_DATA);
    // 2 zeros (unused displays)
    disp_write(0, DISP_MODE_DATA);
    disp_write(0, DISP_MODE_DATA);
}


int main (void) {
    kybd_t kbd_info;
    
    port_init();
    kybd_init();
    disp_init();

    while (1) {
        _delay_ms(20);
        kybd_scan(&kbd_info);
        disp_update(&kbd_info);
    }
}

