/****************************************************************************
 * kybd_led_io.c
 * Keyboard Scan and 7-Segment LED Dispay Updater
 * Version 1.1
 * ---
 * SHORT - Design as the glue logic for the RCU-85 Programming keyboard. 
 *         This will eventually also connect into the 8085A Memory bus and
 *         act as a monitor to program and read RAM Data.
 * 
 * Ver 1.1      Ported to an API to be used by rcu85_monitor
 * 
 * ---
 * TODO PHASES
 * [B] Add ability to HOLD 8085 CPU and directly access the memory space.
 *     ref. Atmel Mega2560 EMI interface or GPIO if EMI is too fast.
 * [C] Serial interface and Monitor CLI. Support for file downloading
 *     X,Y,Z-Modem? into a local RAM buffer then off-loading into RCU-85
 *     Memory.
 ***************************************************************************/

#include "kybd_led_io.h"
#include <avrlib/gpio_api.h>
#include <avrlib/libtime.h>

/* Physical Description of the RCU-85 Keyboard/LED controller
 * Target: ATMega2560 (Arduino Mega2560)
 */

/* Display: ICM7218A -------------------------------------------------
 *
 * 7-Segment Display Configuration
 * +--+--+--+--+  +--+--+
 * |  |  |  |  |  |  |  |
 * |  |  |  |  |  |  |  |
 * +--+--+--+--+  +--+--+
 *  N3 N2 N1 N0    n1 n0
 * +-----+-----+  +-----+
 *   MSB   LSB     Byte
 * +-----------+  +-----+
 *   Address       Data
 *
 * 7-Seg. Display Assignment
 *  D1 - N3
 *  D2 - N2
 *  D3 - N1
 *  D4 - N0
 *  D5 - n1
 *  D6 - n0
 *  D7 - N/U (future)
 *  D8 - N/U (future)
 *
 * ICM7218A Pinning
 * IO0    - A0  PORTF0      DATA
 * IO1    - A1  PORTF1      DATA
 * IO2    - A2  PORTF2      DATA
 * IO3    - A3  PORTF3      DATA
 * IO4    - A4  PORTF4      DATA/CTRL
 * IO5    - A5  PORTF5      CTRL
 * IO6    - A6  PORTF6      CTRL
 * IO7    - A7  PORTF7      CTRL
 * /WRITE - A8  PORTK0      CTRL
 * MODE   - A9  PORTK1      MODE (DATA/CTRL)
 * ------------------------------------------------------------------*/

#define DDATA_PORT      PM_PORT_F
static int hndl_ddata = 0;  /* setup as a port, uses all 8 bits */
#define DDATA_WRT_PORT   PM_PORT_K
#define DDATA_WRT_BIT   PM_PIN_0
static int hndl_dwrt  = 0;   /* port bit */
#define DDATA_MOD_PRT   PM_PORT_K
#define DDATA_MOD_BIT   PM_PIN_1
static int hndl_dmod  = 0;   /* port bit */
static uint8_t disp_setup = 0;

/* ICM7218A Programming ----------------------------
 * Special bit modes on the 'kdata' port
 * Name     Description             Bit
 * -------- ----------------------- ---
 * BSEL     Bank Select             3
 * SHDN     Shutdown                4
 * DECN     Decode/No-Decode        5
 * CODEB    HEX/CODE-B              6
 * STRTU    Start Update            7
 * -------------------------------------------------*/
#define DISP_PIN_BSEL       3
#define DISP_PIN_SHTDN      4
#define DISP_PIN_DEC_NODEC  5
#define DISP_PIN_HEX_CODEB  6
#define DISP_PIN_DAT_START  7
/* Program Bit States ------------------------------
 * DISP_BANK_A          [BSEL]  Select Bank A
 * DISP_BANK_B          [BSEL]  Select Bank B
 * DISP_RUN             [SHDN]  Operate
 * DISP_SHTDN           [SHDN]  Shutdown
 * DISP_NO_DECODE       [DECN]  Do not Decode data
 * DISP_DECODE          [DECN]  Decode data as ...
 * DISP_DEC_HEX         [CODEB] ...HEX
 * DISP_DEC_CODEB       [CODEB] ...Code-B
 * DISP_DAT_START       [STRTU] Start Update
 * Functional Operations ---------------------------
 * DISP_MODE_CTRL       Send a control byte
 * DISP_MODE_DATA       Send a Data bytes
 * DISP_WR_IDLE         /WR line in the idle/inactive sate
 * DISP_WRITE           Set /Write level
 * -------------------------------------------------*/
#define DISP_BANK_A        1
#define DISP_BANK_B        0
#define DISP_RUN           1
#define DISP_SHTDN         0
#define DISP_NO_DECODE     1
#define DISP_DECODE        0
#define DISP_DEC_HEX       1
#define DISP_DEC_CODEB     0
#define DISP_DAT_START     1
/* - */
#define DISP_MODE_CTRL     1
#define DISP_MODE_DATA     0
#define DISP_WR_IDLE       1
#define DISP_WRITE         0  /* active low */
// Predefined CONTROL setups
#define DISP_CTRL_INIT         (DISP_BANK_A << DISP_PIN_BSEL) | (DISP_RUN << DISP_PIN_SHTDN) | (DISP_DEC_HEX << DISP_PIN_HEX_CODEB)
#define DISP_CTRL_START_UPDATE (DISP_BANK_A << DISP_PIN_BSEL) | (DISP_RUN << DISP_PIN_SHTDN) | (DISP_DEC_HEX << DISP_PIN_HEX_CODEB) | (DISP_DAT_START << DISP_PIN_DAT_START)
// Hex Data input on: IO[3..0] !
// Display Databus Timing
#define DISP_TIM_PRE_US     1
#define DISP_TIM_HOLD_US    1
#define DISP_TIM_POST_US    1

// Write a byte to the display
static int disp_write(uint8_t byt, uint8_t mode) {
    int rc = KD_ERR_DISP;
    if (disp_setup) {
        /* setup data/ctrl byte on data-port */
        pm_out(hndl_ddata, byt);
        /* set data-type, '1' = control-byte */
        pm_out(hndl_dmod, mode);
        tm_delay_us(DISP_TIM_PRE_US);
        /* pulse the /WR line */
        pm_out(hndl_dwrt, DISP_WRITE);
        tm_delay_us(DISP_TIM_HOLD_US);
        pm_out(hndl_dwrt, DISP_WR_IDLE);
        tm_delay_us(DISP_TIM_POST_US);
        rc = KD_SUCCESS;
    }
    return rc;
}

// call to setup LED display control
static int disp_init(void) {
    int rc = KD_ERR_DISP;
    hndl_dwrt  = pm_register_pin(DDATA_WRT_PORT, DDATA_WRT_BIT, PINMODE_OUTPUT_HI);
    hndl_dmod  = pm_register_pin(DDATA_MOD_PRT, DDATA_MOD_BIT, PINMODE_OUTPUT_LO);
    hndl_ddata = pm_register_prt(DDATA_PORT, 0, PINMODE_OUTPUT_LO);
    if ((hndl_dwrt > 0) && (hndl_dmod > 0) && (hndl_ddata > 0)) {
        uint8_t disp = DISP_CTRL_INIT;
        disp_setup = 1; // setup completed.
        // setup the ICM7218A for HEX output
        rc = disp_write(disp, DISP_MODE_CTRL);
    }
    return rc;
}

uint8_t disp_isInitializaed(void) {
    return disp_setup;
}

// call to refresh the LED 7-Segment Display
int disp_update(kybd_t * disp) {
    int rc = KD_ERR_DISP;
    if (disp && disp_setup) {
    do {
        uint8_t dd = DISP_CTRL_START_UPDATE;
        if ((rc = disp_write(dd, DISP_MODE_CTRL)) != KD_SUCCESS) break;
        dd = (disp->ds.dat.addr_hi >> 4) & 0x0f;    // A[15..12]
        if ((rc = disp_write(dd, DISP_MODE_DATA)) != KD_SUCCESS) break;
        dd = disp->ds.dat.addr_hi & 0x0f;           // A[11..8]
        if ((rc = disp_write(dd, DISP_MODE_DATA)) != KD_SUCCESS) break;
        dd = (disp->ds.dat.addr_lo >> 4) & 0x0f;    // A[7..4]
        if ((rc = disp_write(dd, DISP_MODE_DATA)) != KD_SUCCESS) break;
        dd = disp->ds.dat.addr_lo & 0x0f;           // A[3..0]
        if ((rc = disp_write(dd, DISP_MODE_DATA)) != KD_SUCCESS) break;
        dd = (disp->ds.dat.data >> 4) & 0x0f;       // D[7..4]
        if ((rc = disp_write(dd, DISP_MODE_DATA)) != KD_SUCCESS) break;
        dd = disp->ds.dat.data & 0x0f;              // D[3..0]
        if ((rc = disp_write(dd, DISP_MODE_DATA)) != KD_SUCCESS) break;
        // 2 zeros (unused displays)
        if ((rc = disp_write(0, DISP_MODE_DATA)) != KD_SUCCESS) break;
        rc = disp_write(0, DISP_MODE_DATA);
    } while (0);
    }
    return rc;
}


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

#define KYBDCRTL_LD_PRT PM_PORT_K
#define KYBDCRTL_LD_PIN PM_PIN_4
#define KYBDCRTL_LD_MOD PINMODE_OUTPUT_HI
static int hndl_kybd_ld = 0;
#define KYBDCRTL_CK_PRT PM_PORT_K
#define KYBDCRTL_CK_PIN PM_PIN_5
#define KYBDCRTL_CK_MOD PINMODE_OUTPUT_LO
static int hndl_kybd_clk = 0;
#define KYBDDATA_PRT    PM_PORT_K
#define KYBDDATA_PIN    PM_PIN_6
#define KYBDDATA_MOD    PINMODE_INPUT_TRI
static int hndl_kybd_din = 0;
static int kybd_setup = 0;

#define KYBD_SHIFTER_COUNT  3
#define KYBD_LOAD_DLY_US    1
#define KYBD_CLK_PD_US      1

// call to setup keyboard scanning
static int kybd_init(void) {
    // LOAD: OUT-high, CLK: OUT-low, DIN: IN-Hi-z
    int rc = KD_ERR_KYBD;
    hndl_kybd_ld  = pm_register_pin(KYBDCRTL_LD_PRT, KYBDCRTL_LD_PIN, KYBDCRTL_LD_MOD);
    hndl_kybd_clk = pm_register_pin(KYBDCRTL_CK_PRT, KYBDCRTL_CK_PIN, KYBDCRTL_CK_MOD);
    hndl_kybd_din = pm_register_pin(KYBDDATA_PRT, KYBDDATA_PIN, KYBDDATA_MOD);
    if ((hndl_kybd_ld > PM_SUCCESS) && (hndl_kybd_clk > PM_SUCCESS) && (hndl_kybd_din > PM_SUCCESS)) {
        kybd_setup = 1; // setup completed.
        rc = KD_SUCCESS;
    }
    return rc;
}

uint8_t kybd_isInitializaed(void) {
    return kybd_setup;
}

static int kybd_clk(void) {
    int rc = KD_ERR_KYBD;
    if (kybd_setup) {
        int grc = PM_ERROR;
        do {
            if ((grc = pm_tog(hndl_kybd_clk)) != PM_SUCCESS) break;
            tm_delay_us(KYBD_CLK_PD_US);
            if ((grc = pm_tog(hndl_kybd_clk)) != PM_SUCCESS) break;
            tm_delay_us(KYBD_CLK_PD_US);
        } while (0);
        rc = (grc == PM_SUCCESS) ? KD_SUCCESS : KD_ERR_KYBD;
    }
    return rc;
}

static int kybd_load(void) {
    int rc = KD_ERR_KYBD;
    if (kybd_setup) {
        int grc = PM_ERROR;
        do {
            if ((grc = pm_tog(hndl_kybd_ld)) != PM_SUCCESS) break;
            tm_delay_us(KYBD_LOAD_DLY_US);
            if ((grc = pm_tog(hndl_kybd_ld)) != PM_SUCCESS) break;
            tm_delay_us(KYBD_LOAD_DLY_US);
        } while (0);
        rc = (grc == PM_SUCCESS) ? KD_SUCCESS : KD_ERR_KYBD;
    }
    return rc;
}

// call to invoke one key-scan. Blocks until done.
int kybd_scan(kybd_t * kdata) {
    int rc = KD_ERR_KYBD;
    if (kybd_setup) {
        if ((rc = kybd_load()) == KD_SUCCESS) {
            uint8_t grc,d,p,b;
            for (p = 0 ; p < KYBD_SHIFTER_COUNT ; ++p) {
                d = 0;
                // shift 7 times, save on the last (8th) iteration
                for (b = 0 ; b < 8 ; ++b) {
                    grc = pm_in(hndl_kybd_din);
                    if (grc >= 0) {
                        if (grc) {
                            d |= 0x80; // DIN ~ '1'
                        }
                        // 8-th run does not shift the data, it is saved instead.
                        if (b < 7) {
                            d = d >> 1;
                        } else {
                            kdata->ds.byt[p] = d;
                        }
                        kybd_clk(); // 1 .. 7
                    } else {
                        rc = KD_ERR_KYBD;
                        break;
                    }
                } // for (b)
                if (rc == KD_ERR_KYBD) {
                    break; // fail out of outer loop
                }
            } // for(p)
        } // load ok
    } // keyboard configured ok
    return rc;
}

int kybdio_sysinit(void) {
    int rc;
    if ( !pm_isInitialized() )
        pm_init();
    rc = kybd_init();
    if (!rc)
        rc = disp_init();
    return rc;
}
