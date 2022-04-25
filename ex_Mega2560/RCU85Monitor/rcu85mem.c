/*********************************************************************
 * rcu85mem.c
 * 
 * Version 1.0
 * ---
 * RCU85 Memory Manager and API
 * 
 * Command List:
 *  
 **********************************************************************/

#include "rcu85mem.h"
#include <avrlib/gpio_api.h>
#include <avrlib/libtime.h>

#define ADDRHI_PORT      PM_PORT_C
#define ADDRHI_MODE_STBY PINMODE_INPUT_TRI   /* resting-state should be tri-state/input */
#define ADDRHI_MODE_ACT  PINMODE_OUTPUT_LO
static int hndl_addrhi = 0;
#define ADDRDATA_PORT      PM_PORT_A
#define ADDRDATA_MODE_STBY PINMODE_INPUT_TRI   /* resting-state should be tri-state/input */
#define ADDRDATA_MODE_ACT  PINMODE_OUTPUT_LO
static int hndl_addrdata = 0;
#define EXTSEL_PORT     PM_PORT_K
#define EXTSEL_PIN      PM_PIN_7
#define EXTSEL_MODE     PINMODE_OUTPUT_HI
#define EXTSEL_INT      1                       /* allow CPU to control CE's */
#define EXTSEL_EXT      0                       /* gain access to CE controller */
static int hndl_extsel = 0;
#define WR_PORT         PM_PORT_G
#define WR_PIN          PM_PIN_0
#define WR_MODE_ACT     PINMODE_OUTPUT_HI
#define WR_MODE_STBY    PINMODE_INPUT_TRI
#define WR_MODE_ON      0
#define WR_MODE_OFF     1
static int hndl_wr = 0;
#define RD_PORT         PM_PORT_G
#define RD_PIN          PM_PIN_1
#define RD_MODE_ACT     PINMODE_OUTPUT_HI
#define RD_MODE_STBY    PINMODE_INPUT_TRI
#define RD_MODE_ON      0
#define RD_MODE_OFF     1
static int hndl_rd = 0;
#define ALE_PORT        PM_PORT_G
#define ALE_PIN         PM_PIN_2
#define ALE_MODE_ACT    PINMODE_OUTPUT_LO
#define ALE_MODE_STBY   PINMODE_INPUT_TRI
#define ALE_MODE_ADDR    1
#define ALE_MODE_DATA    0
static int hndl_ale = 0;
#define IOM_PORT        PM_PORT_L
#define IOM_PIN         PM_PIN_0
#define IOM_MODE_ACT    PINMODE_OUTPUT_LO
#define IOM_MODE_STBY   PINMODE_INPUT_TRI
#define IOM_MODE_IO     1
#define IOM_MODE_MEM    0
static int hndl_iom = 0;
#define HOLD_PORT       PM_PORT_L
#define HOLD_PIN        PM_PIN_1
#define HOLD_MODE       PINMODE_OUTPUT_LO
#define HOLD_REQ        1
#define HOLD_CLR        0
static int hndl_hold = 0;
#define HLDA_PORT       PM_PORT_L
#define HLDA_PIN        PM_PIN_2
#define HLDA_MODE       PINMODE_INPUT_TRI
#define HLDA_BUSY       0
#define HLDA_ACK        1
static int hndl_hold_ack = 0;
#define RESET_PORT      PM_PORT_L
#define RESET_PIN       PM_PIN_3
#define RESET_MODE      PINMODE_OUTPUT_HI
#define RESET_ON        0
#define RESET_OFF       1
static int hndl_reset = 0;

static uint8_t  isInitializaed = 0;
static uint8_t  isHeld = 0;

/* --- Internal Operations -------------------------------------------*/



/* --- API -----------------------------------------------------------*/

int rcmem_init(void) {
    int rc = RCM_ERROR;
    if (!isInitializaed) {
        if (!pm_isInitialized()) {
            pm_init();
        }
        hndl_addrhi   = pm_register_prt(ADDRHI_PORT, 0, ADDRHI_MODE_STBY);
        hndl_addrdata = pm_register_prt(ADDRDATA_PORT, 0, ADDRDATA_MODE_STBY);
        hndl_extsel   = pm_register_pin(EXTSEL_PORT, EXTSEL_PIN, EXTSEL_MODE);
        hndl_wr       = pm_register_pin(WR_PORT, WR_PIN, WR_MODE_STBY);
        hndl_rd       = pm_register_pin(RD_PORT, RD_PIN, RD_MODE_STBY);
        hndl_ale      = pm_register_pin(ALE_PORT, ALE_PIN, ALE_MODE_STBY);
        hndl_iom      = pm_register_pin(IOM_PORT, IOM_PIN, IOM_MODE_STBY);
        hndl_hold     = pm_register_pin(HOLD_PORT, HOLD_PIN, HOLD_MODE);
        hndl_hold_ack = pm_register_pin(HLDA_PORT, HLDA_PIN, HLDA_MODE);
        hndl_reset    = pm_register_pin(RESET_PORT, RESET_PIN, RESET_MODE);
        if ((hndl_addrhi > 0) && (hndl_addrdata > 0) && (hndl_extsel > 0) &&
          (hndl_wr > 0) && (hndl_rd > 0) && (hndl_ale > 0) && (hndl_iom > 0) &&
          (hndl_hold > 0) && (hndl_hold_ack > 0) && (hndl_reset > 0)) {
            isInitializaed = 1;
            isHeld = 0;
            rc = RCM_SUCCESS;
        }
    }
    return rc;
}

uint8_t rcmem_isHeld(void) {
    return isHeld;
}

int rcmem_hold(void) {
    int rc = RCM_ERROR;
    if (!rcmem_isHeld()) {
        int grc;
        int t_max = 100;
        pm_out(hndl_hold, HOLD_REQ);
        while ( (grc = pm_in(hndl_hold_ack)) == HLDA_BUSY ) {
            if (--t_max == 0) {
                rc = RCM_TIMEOUT; /* timeout */
                break;
            }
            tm_delay_us(10);
        }
        if (grc == HLDA_ACK) {
            /* activate addr, data, ctrl busses */
            pm_chg_dir(hndl_wr, WR_MODE_ACT);               /* high */
            pm_chg_dir(hndl_rd, RD_MODE_ACT);               /* high */
            pm_chg_dir(hndl_ale, ALE_MODE_ACT);             /* low */
            pm_chg_dir(hndl_iom, IOM_MODE_ACT);             /* low (mem) */
            pm_chg_dir(hndl_addrhi, ADDRHI_MODE_ACT);       /* 0x00 */
            pm_chg_dir(hndl_addrdata, ADDRDATA_MODE_ACT);   /* 0x00 */
            isHeld = 1;
            rc = RCM_SUCCESS;
        }
    }
    return rc;
}

int rcmem_release(uint8_t doReset) {
    int rc = RCM_ERROR;
    if (rcmem_isHeld()) {
        int grc;
        int t_max = 100;
        if (doReset) {
            pm_out(hndl_reset, RESET_ON);
            tm_delay_us(1);
        }
        /* ensure all data, ctrl lines are input Hi-Z */
        pm_out(hndl_extsel, EXTSEL_INT); /* release the CE controller */
        pm_chg_dir(hndl_wr, WR_MODE_STBY);
        pm_chg_dir(hndl_rd, RD_MODE_STBY);
        pm_chg_dir(hndl_ale, ALE_MODE_STBY);
        pm_chg_dir(hndl_iom, IOM_MODE_STBY);
        pm_chg_dir(hndl_addrhi, ADDRHI_MODE_STBY);
        pm_chg_dir(hndl_addrdata, ADDRDATA_MODE_STBY);
        /* release the bus - wait for ack to clear */
        pm_out(hndl_hold, HOLD_CLR);
        while ( (grc = pm_in(hndl_hold_ack)) == HLDA_ACK ) {
            if (--t_max == 0) {
                rc = RCM_TIMEOUT; /* timeout */
                break;
            }
            tm_delay_us(10);
        }
        if (grc == HLDA_BUSY) {
            if (doReset) {
                pm_out(hndl_reset, RESET_OFF);
            }
            isHeld = 0;
            rc = RCM_SUCCESS;
        }
    }
    return rc;
}

typedef enum bus_act_type {
    BUS_ACT_RD = 0,
    BUS_ACT_WR,
    /* ... */
    BUS_ACT_COUNT
} bus_act_t;

static int bus_action(uint16_t addr, uint8_t * data, uint16_t len, uint8_t isIO, bus_act_t act) {
    int rc = RCM_ERROR;
    uint16_t idx = 0;
    if (isHeld && data && len) {
        uint8_t addr_lo = (uint8_t)(addr & 0xff);
        uint8_t addr_hi = (isIO) ? addr_lo : (uint8_t)(addr >> 8);
        /* (1) set mem or I/O */
        pm_out(hndl_iom, (isIO)?IOM_MODE_IO:IOM_MODE_MEM);
        /* loop for every byte in or out */
        for ( idx = 0 ; idx < len ; ++idx ) {
            /* (2) set addr_lo, addr_hi (note: for I/O, (A7 - A0) =copy=> (A15 - A8) */
            if (pm_dir(hndl_addrdata) == PINDIR_INPUT)
                pm_chg_dir(hndl_addrdata, ADDRDATA_MODE_ACT);
            pm_out(hndl_addrdata, addr_lo);
            pm_out(hndl_addrhi, addr_hi);
            /* (3) enable CE controller (EXT_SEL) */
            pm_out(hndl_extsel, EXTSEL_EXT);
            /* (4) set ALE */
            pm_out(hndl_ale, ALE_MODE_ADDR);
            /* (5) clr ALE */
            pm_out(hndl_ale, ALE_MODE_DATA);
            /* WRITE / READ */
            if (act == BUS_ACT_WR) {
                /* (7) [WR] : assert WR */
                pm_out(hndl_wr, WR_MODE_ON);
                /* (8) wait 1 us */
                tm_delay_us(1);
                /* set data on bus */
                pm_out(hndl_addrdata, data[idx]);
                /* wait 1 us */
                tm_delay_us(1);
                /* (9) clr WR */
                pm_out(hndl_wr, WR_MODE_OFF);
            } else { /* read */
                /* change AD [0..7] to be Tri-state *before* asserting RD */
                if (pm_dir(hndl_addrdata) == PINDIR_OUTPUT)
                    pm_chg_dir(hndl_addrdata, ADDRDATA_MODE_STBY);
                /* (7) [RD] : assert [RD] */
                pm_out(hndl_rd, RD_MODE_ON);
                /* (8) wait 1 us */
                tm_delay_us(1);
                /* read data off bus */
                data[idx] = pm_in(hndl_addrdata);
                /* (9) clr RD */
                pm_out(hndl_rd, RD_MODE_OFF);
            }
            /* release the C controller */
            pm_out(hndl_extsel, EXTSEL_INT);
            /* increment and re-calculate addresses */
            addr ++;
            addr_lo = (uint8_t)(addr & 0xff);
            addr_hi = (isIO) ? addr_lo : (uint8_t)(addr >> 8);
        } // for ()
        rc = (int)idx; // return the byte count transacted.
    } // args good
    return rc;
}

int rcmem_write(uint16_t addr, uint8_t * data, uint16_t len, uint8_t isIO) {
    return bus_action(addr, data, len, isIO, BUS_ACT_WR);
}

int rcmem_read(uint16_t addr, uint8_t * data, uint16_t len, uint8_t isIO) {
    return bus_action(addr, data, len, isIO, BUS_ACT_RD);
}
