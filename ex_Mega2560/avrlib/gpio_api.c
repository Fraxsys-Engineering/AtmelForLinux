/**********************************************************************
 * gpio_api.c
 *
 * GPIO API
 * Register for usage of a GPIO pin (single bit/wire) or for an entire
 * 8-bit PORT. In BIT mode, only the registered GPIO pin is affected and
 * other systems may register for other pins in the same port. In PORT
 * mode, the registering system has control over all 8 pins in the port.
 *
 * GPIO pin, port accounting is done in this API and an error is flagged
 * if another subsystem attempts to register a previously registered
 * pin or port.
 *
 * Non-GPIO libraries may use the accounting system in the GPIO API to
 * register and "lock" GPIO pins or ports for non-gpio usage and prevent
 * an attempt to subvert a pin operating as eg. SPI into a regular GPIO
 * pin. It is only necessary for the other lib to register the pin in
 * order to lock it.
 * (!) Non-GPIO Registration: Use PINMODE_UNCHANGED for the 'mode'.
 *
 * EXTERNAL DEFINITIONS:
 * 
 * MAX_GPIO_RSVD (40)       Set the maximum number of GPIO reservations
 *                          to be used by the application. Reducing this
 *                          value to the total # of GPIO pins and ports
 *                          to be used by the application will save RAM
 *                          space from being wasted.
 *
 *********************************************************************/

#include "gpio_api.h"

#ifdef EMULATE_LIB
 #include <stdio.h>
 uint8_t MCUCR = 0;
 #define PUD 4
 uint8_t TEST_REGISTERS[TEST_REGR_COUNT];
 sfr8p_t TR_ADDR_BASE = (sfr8p_t)&(TEST_REGISTERS[0]);
 #define GP_ADDR_OFFSET  TR_ADDR_BASE
 #define PINA /* use for pin testing */
 #define PINB /* use for port testing */
#else
 #include <avr/io.h>
 #define GP_ADDR_OFFSET  0x20
#endif

#ifndef MAX_GPIO_RSVD
#define MAX_GPIO_RSVD 40
#endif

/* GPIO REGISTER ADDRESSES - NOT DEFINED IN AVR HEADERS!!! -----------*/
#define GP_A_PIN    (GP_ADDR_OFFSET+0x00)
#define GP_A_DDR    (GP_ADDR_OFFSET+0x01)
#define GP_A_PRT    (GP_ADDR_OFFSET+0x02)
/* - */
#define GP_B_PIN    (GP_ADDR_OFFSET+0x03)
#define GP_B_DDR    (GP_ADDR_OFFSET+0x04)
#define GP_B_PRT    (GP_ADDR_OFFSET+0x05)
/* - */
#define GP_C_PIN    (GP_ADDR_OFFSET+0x06)
#define GP_C_DDR    (GP_ADDR_OFFSET+0x07)
#define GP_C_PRT    (GP_ADDR_OFFSET+0x08)
/* - */
#define GP_D_PIN    (GP_ADDR_OFFSET+0x09)
#define GP_D_DDR    (GP_ADDR_OFFSET+0x0A)
#define GP_D_PRT    (GP_ADDR_OFFSET+0x0B)
/* - */
#define GP_E_PIN    (GP_ADDR_OFFSET+0x0C)
#define GP_E_DDR    (GP_ADDR_OFFSET+0x0D)
#define GP_E_PRT    (GP_ADDR_OFFSET+0x0E)
/* - */
#define GP_F_PIN    (GP_ADDR_OFFSET+0x0F)
#define GP_F_DDR    (GP_ADDR_OFFSET+0x10)
#define GP_F_PRT    (GP_ADDR_OFFSET+0x11)
/* - */
#define GP_G_PIN    (GP_ADDR_OFFSET+0x12)
#define GP_G_DDR    (GP_ADDR_OFFSET+0x13)
#define GP_G_PRT    (GP_ADDR_OFFSET+0x14)
/* - */
#define GP_H_PIN    (0x100)
#define GP_H_DDR    (0x101)
#define GP_H_PRT    (0x102)
/* - */
#define GP_J_PIN    (0x103)
#define GP_J_DDR    (0x104)
#define GP_J_PRT    (0x105)
/* - */
#define GP_K_PIN    (0x106)
#define GP_K_DDR    (0x107)
#define GP_K_PRT    (0x108)
/* - */
#define GP_L_PIN    (0x109)
#define GP_L_DDR    (0x10A)
#define GP_L_PRT    (0x10B)
/* -------------------------------------------------------------------*/


typedef struct s_pinstatus_type {
    uint8_t     bf_dir      : 3;    /* PINMODE_***          */
    uint8_t     bf_exist    : 1;    /* pin exists? <-- 1    */
    uint8_t     bf_locked   : 1;    /* registered? <-- 1    */
    uint8_t     bf_state    : 1;    /* last r/w state       */
    uint8_t     bf_reserved : 2;    /* unused               */ 
} s_pinstatus;

typedef struct s_portstatus_type {
    s_pinstatus pin[8];             /* pin/bit 0 .. 7       */
    uint8_t     port_mask;  /* filter write outputs through a ddr mask (all output pins are '1') */
    uint8_t     pup_mask;   /* pullup_mask - all bit inputs w/ pullup on must have a '1' in corresponding bitfield */
    sfr8p_t     rddr;
    sfr8p_t     rpin;
    sfr8p_t     rport;   
} s_portstatus;

static s_portstatus port_stat[PORT_COUNT] = {0};    /* PORT A..L */

#define PINIDX_PORT 0xff        /* when 'pinidx' is set to this value then the entire port is being used. */
typedef struct s_handle_type {
    s_portstatus *  p_port;
    uint8_t         pinidx;
    uint8_t         port_state; /* (for ports only) last r/w port state */
} s_handle;

/* Internal Registration List ----------------------------------------*/
static s_handle reglist[MAX_GPIO_RSVD] = {0};
static uint16_t s_nxt_hndl = 1; /* 1st valid handle # is 1. handles are dec-by-1 before referecing the port_stat[] list internally. */
/* -------------------------------------------------------------------*/

/* Initialization State ----------------------------------------------*/
static uint8_t pm_was_initialized = 0;
/* -------------------------------------------------------------------*/

uint8_t pm_isInitialized(void) {
    return pm_was_initialized;
}

void pm_init(void) {
    // Setup global pin registration tables
    uint8_t i;
#ifdef EMULATE_LIB
    for ( i = 0 ; i < TEST_REGR_COUNT ; ++i ) {
        TEST_REGISTERS[i] = 0;
    }
#endif
#ifdef PINA
    port_stat[PM_PORT_A].rpin = (sfr8p_t)GP_A_PIN;
    port_stat[PM_PORT_A].rddr = (sfr8p_t)GP_A_DDR;
    port_stat[PM_PORT_A].rport= (sfr8p_t)GP_A_PRT;
    port_stat[PM_PORT_A].port_mask = 0;
    port_stat[PM_PORT_A].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_A].pin[i].bf_exist = 1;
        port_stat[PM_PORT_A].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINB
    port_stat[PM_PORT_B].rpin = (sfr8p_t)GP_B_PIN;
    port_stat[PM_PORT_B].rddr = (sfr8p_t)GP_B_DDR;
    port_stat[PM_PORT_B].rport= (sfr8p_t)GP_B_PRT;
    port_stat[PM_PORT_B].port_mask = 0;
    port_stat[PM_PORT_B].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_B].pin[i].bf_exist = 1;
        port_stat[PM_PORT_B].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINC
    port_stat[PM_PORT_C].rpin = (sfr8p_t)GP_C_PIN;
    port_stat[PM_PORT_C].rddr = (sfr8p_t)GP_C_DDR;
    port_stat[PM_PORT_C].rport= (sfr8p_t)GP_C_PRT;
    port_stat[PM_PORT_C].port_mask = 0;
    port_stat[PM_PORT_C].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_C].pin[i].bf_exist = 1;
        port_stat[PM_PORT_C].pin[i].bf_locked = 0;
    }
#endif
#ifdef PIND
    port_stat[PM_PORT_D].rpin = (sfr8p_t)GP_D_PIN;
    port_stat[PM_PORT_D].rddr = (sfr8p_t)GP_D_DDR;
    port_stat[PM_PORT_D].rport= (sfr8p_t)GP_D_PRT;
    port_stat[PM_PORT_D].port_mask = 0;
    port_stat[PM_PORT_D].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_D].pin[i].bf_exist = 1;
        port_stat[PM_PORT_D].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINE
    port_stat[PM_PORT_E].rpin = (sfr8p_t)GP_E_PIN;
    port_stat[PM_PORT_E].rddr = (sfr8p_t)GP_E_DDR;
    port_stat[PM_PORT_E].rport= (sfr8p_t)GP_E_PRT;
    port_stat[PM_PORT_E].port_mask = 0;
    port_stat[PM_PORT_E].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_E].pin[i].bf_exist = 1;
        port_stat[PM_PORT_E].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINF
    port_stat[PM_PORT_F].rpin = (sfr8p_t)GP_F_PIN;
    port_stat[PM_PORT_F].rddr = (sfr8p_t)GP_F_DDR;
    port_stat[PM_PORT_F].rport= (sfr8p_t)GP_F_PRT;
    port_stat[PM_PORT_F].port_mask = 0;
    port_stat[PM_PORT_F].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_F].pin[i].bf_exist = 1;
        port_stat[PM_PORT_F].pin[i].bf_locked = 0;
    }
#endif
#ifdef PING
    port_stat[PM_PORT_G].rpin = (sfr8p_t)GP_G_PIN;
    port_stat[PM_PORT_G].rddr = (sfr8p_t)GP_G_DDR;
    port_stat[PM_PORT_G].rport= (sfr8p_t)GP_G_PRT;
    port_stat[PM_PORT_G].port_mask = 0;
    port_stat[PM_PORT_G].pup_mask = 0;
    for ( i = 0 ; i < 6 ; ++i ) {
        port_stat[PM_PORT_G].pin[i].bf_exist = 1;
        port_stat[PM_PORT_G].pin[i].bf_locked = 0;
    }
    for ( i = 6 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_G].pin[i].bf_exist = 0;
        port_stat[PM_PORT_G].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINH
    port_stat[PM_PORT_H].rpin = (sfr8p_t)GP_H_PIN;
    port_stat[PM_PORT_H].rddr = (sfr8p_t)GP_H_DDR;
    port_stat[PM_PORT_H].rport= (sfr8p_t)GP_H_PRT;
    port_stat[PM_PORT_H].port_mask = 0;
    port_stat[PM_PORT_H].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_H].pin[i].bf_exist = 1;
        port_stat[PM_PORT_H].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINJ
    port_stat[PM_PORT_J].rpin = (sfr8p_t)GP_J_PIN;
    port_stat[PM_PORT_J].rddr = (sfr8p_t)GP_J_DDR;
    port_stat[PM_PORT_J].rport= (sfr8p_t)GP_J_PRT;
    port_stat[PM_PORT_J].port_mask = 0;
    port_stat[PM_PORT_J].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_J].pin[i].bf_exist = 1;
        port_stat[PM_PORT_J].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINK
    port_stat[PM_PORT_K].rpin = (sfr8p_t)GP_K_PIN;
    port_stat[PM_PORT_K].rddr = (sfr8p_t)GP_K_DDR;
    port_stat[PM_PORT_K].rport= (sfr8p_t)GP_K_PRT;
    port_stat[PM_PORT_K].port_mask = 0;
    port_stat[PM_PORT_K].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_K].pin[i].bf_exist = 1;
        port_stat[PM_PORT_K].pin[i].bf_locked = 0;
    }
#endif
#ifdef PINL
    port_stat[PM_PORT_L].rpin = (sfr8p_t)GP_L_PIN;
    port_stat[PM_PORT_L].rddr = (sfr8p_t)GP_L_DDR;
    port_stat[PM_PORT_L].rport= (sfr8p_t)GP_L_PRT;
    port_stat[PM_PORT_L].port_mask = 0;
    port_stat[PM_PORT_L].pup_mask = 0;
    for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
        port_stat[PM_PORT_L].pin[i].bf_exist = 1;
        port_stat[PM_PORT_L].pin[i].bf_locked = 0;
    }
#endif
    s_nxt_hndl = 1;
    pm_was_initialized = 1;
}

int pm_glb_pup_control(uint8_t pupctrl) {
    if (pupctrl)
        MCUCR = MCUCR | (1<<PUD);
    else
        MCUCR = MCUCR & (uint8_t)~(1<<PUD);
    return PM_SUCCESS;
}

static void s_setmode_pin(s_portstatus * pp, uint8_t pin, uint8_t mode) {
    sfr8p_t rddr = pp->rddr;
    sfr8p_t rport = pp->rport;
    switch (mode) {
    case PINMODE_INPUT_TRI:
        *rddr = *rddr & (uint8_t)~(1<<pin);     /* set 0:input                  */
        *rport = *rport & (uint8_t)~(1<<pin);   /* write to port to clr pullup  */
        pp->port_mask = pp->port_mask & (uint8_t)~(1<<pin);
        pp->pup_mask = pp->pup_mask & (uint8_t)~(1<<pin);
        break;
    case PINMODE_INPUT_PU:
        *rddr = *rddr & (uint8_t)~(1<<pin);     /* set 0:input                  */
        *rport = *rport | (1<<pin);             /* write to port to set pullup  */
        pp->port_mask = pp->port_mask & (uint8_t)~(1<<pin);
        pp->pup_mask = pp->pup_mask | (1<<pin); /* this mode is the only one where the pullup-mask bit is set */
        break;
    case PINMODE_OUTPUT_LO:
        *rddr = *rddr | (1<<pin);               /* set 1:output                 */
        *rport = *rport & (uint8_t)~(1<<pin);   /* set pin low                  */
        pp->port_mask = pp->port_mask | (1<<pin);
        pp->pup_mask = pp->pup_mask & (uint8_t)~(1<<pin);
        break;
    case PINMODE_OUTPUT_HI:
        *rddr = *rddr | (1<<pin);               /* set 1:output                 */
        *rport = *rport | (1<<pin);             /* set pin high                 */
        pp->port_mask = pp->port_mask | (1<<pin);
        pp->pup_mask = pp->pup_mask & (uint8_t)~(1<<pin);
        break;
    default:
        break;
    }
}

static void s_setmode_port(s_portstatus * pp, uint8_t byt, uint8_t mode) {
    sfr8p_t rddr = pp->rddr;
    sfr8p_t rport = pp->rport;
    switch (mode) {
    case PINMODE_INPUT_TRI:
        *rddr = 0;                  /* set 0:input                  */
        *rport = 0;                 /* write to port to clr pullup  */
        pp->port_mask = 0;
        pp->pup_mask =0;
        break;
    case PINMODE_INPUT_PU:
        *rddr = 0;                  /* set 0:input                  */
        *rport = 0xff;              /* set *ALL* pullups            */
        pp->port_mask = 0;
        pp->pup_mask = 0xff;
        break;
    case PINMODE_OUTPUT_LO:
    case PINMODE_OUTPUT_HI:
        *rddr = 0xff;               /* set 1:output                 */
        *rport = byt;               /* set pins as per 'byt'        */
        pp->port_mask = 0xff;
        pp->pup_mask = 0;
        break;
    default:
        break;
    }
}

int pm_register_pin(uint8_t port, uint8_t pin, uint8_t mode) {
    int rc = PM_ERROR;
    //printf("[pm_register_pin] - port[%d] pin[%d] mode[%d]\n", port, pin, mode );
    if ((port < PORT_COUNT) && (pin < PM_TOTALPINS) && (mode < PINMODE_TOTALMODES) && (s_nxt_hndl <= MAX_GPIO_RSVD)) {
        if (port_stat[port].pin[pin].bf_exist && (port_stat[port].pin[pin].bf_locked == 0)) {
            //printf("    pin is free... using handle[%d]\n", s_nxt_hndl);
            rc = s_nxt_hndl;
            s_nxt_hndl ++;
            rc --; /* (!!!) correct rc to be an index, adjust it later back to a handle (1+) */
            reglist[rc].p_port = &(port_stat[port]);
            reglist[rc].pinidx = pin;
            reglist[rc].port_state = 0; /* not used for pins */
            /* setup pin */
            reglist[rc].p_port->pin[pin].bf_locked = 1;
            reglist[rc].p_port->pin[pin].bf_dir = mode;
            s_setmode_pin(reglist[rc].p_port, pin, mode);
            rc ++; /* return rc back to a handle # */
        }
    }
    return rc;
}

int pm_register_prt(uint8_t port, uint8_t byt, uint8_t mode) {
    int rc = PM_ERROR;
    if ((port < PORT_COUNT) && (mode < PINMODE_TOTALMODES) && (s_nxt_hndl <= MAX_GPIO_RSVD)) {
        /* check all pins in this port to see if they are all free */
        uint8_t i, locked = 0;
        for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
            if (port_stat[port].pin[i].bf_exist && port_stat[port].pin[i].bf_locked) {
                locked = 1;
                break;
            }
        }
        if (locked == 0) {
            /* entire port is free */
            rc = s_nxt_hndl;
            s_nxt_hndl ++;
            rc --; /* (!!!) correct rc to be an index, adjust it later back to a handle (1+) */
            reglist[rc].p_port = &(port_stat[port]);
            reglist[rc].pinidx = PINIDX_PORT;   /* using the entire port */
            reglist[rc].port_state = 0;
            /* lock all the existing pins in the port */
            for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
                if (port_stat[port].pin[i].bf_exist) {
                    port_stat[port].pin[i].bf_locked = 1;
                    port_stat[port].pin[i].bf_dir = mode;
                }
            }
            /* setup port */
            s_setmode_port(reglist[rc].p_port, byt, mode);
            rc ++; /* return rc back to a handle # */
        }
    }
    return rc;
}

static s_handle * s_findhndl(int hndl) {
    s_handle * shndl = NULL;
    if ((hndl > 0) && (hndl < s_nxt_hndl)) {
        shndl = &(reglist[hndl-1]);
    }
    return shndl;
}

int pm_chg_dir(int hndl, uint8_t mode) {
    int rc = PM_ERROR;
    s_handle * rlst = s_findhndl(hndl);
    if (rlst) {
        s_portstatus * pp = rlst->p_port;
        if (rlst->pinidx == PINIDX_PORT) {
            uint8_t i;
            /* is entire port - use current port (last read) status when switching to output */
            for ( i = 0 ; i < PM_TOTALPINS ; ++i ) {
                if (pp->pin[i].bf_exist) {
                    pp->pin[i].bf_dir = mode;
                }
            }
            s_setmode_port(pp, rlst->port_state, mode);
        } else {
            /* is a pin */
            pp->pin[rlst->pinidx].bf_dir = mode;
            s_setmode_pin(pp, rlst->pinidx, mode);
        }
        rc = PM_SUCCESS;
    }
    return rc;
}

int pm_out(int hndl, uint8_t value) {
    int rc = PM_ERROR;
    s_handle * rlst = s_findhndl(hndl);
    //printf("[pm_out(%d)] val[%d] - handle[%016lx]\n", hndl, value, (unsigned long)rlst);
    if (rlst) {
        s_portstatus * pp = rlst->p_port;
        uint8_t pidx = rlst->pinidx;
        uint8_t rmask = (pidx == PINIDX_PORT) ? 0 : (uint8_t)~(1<<pidx); /* rmask - if writing entire port then throw everything away that was initially read */
        uint8_t pval  = *(pp->rport) & rmask & pp->port_mask; /* read port, ignore bit-of-intrest (rmask), retain other outputs */
        uint8_t wval = (pidx == PINIDX_PORT) 
            ? value 
            : (value) 
                ? (1<<pidx) 
                : 0; /* value to write is either entire byte or bit-shifted '1' (or '0') */
    
        //printf("    pidx[%02x] rmask[%02x] port_mask[%02x] pullup_mask[%02x] prev. port-value[%02x] write-value[%02x]\n",
        //    pidx, rmask, pp->port_mask, pp->pup_mask, *(pp->rport), wval);
                
        pval = pval | wval | pp->pup_mask; /* modified value to write is comprised of adjacent out-bits and any input w/ pullups set */
        
        //printf("    mod. port-value[%02x]\n", pval);
        
        *(pp->rport) = pval;
        rc = PM_SUCCESS;
    }
    return rc;
}

// Read the PINx register to get a stable (delayed) value.
int pm_in(int hndl) {
    int rc = PM_ERROR;
    s_handle * rlst = s_findhndl(hndl);
    if (rlst) {
        s_portstatus * pp = rlst->p_port;
        uint8_t pidx = rlst->pinidx;
        if (pidx == PINIDX_PORT) {
            rc = *(pp->rpin);
        } else {
            rc = (*(pp->rpin) & (1<<pidx)) ? 1 : 0;
        }
    }
    return rc;
}

// Write '1' to PINx to toggle the pin in corresponding bit lane
int pm_tog(int hndl) {
    int rc = PM_ERROR;
    s_handle * rlst = s_findhndl(hndl);
    if (rlst) {
        s_portstatus * pp = rlst->p_port;
        uint8_t pidx = rlst->pinidx;
        uint8_t wmask = (pidx == PINIDX_PORT) ? 0xff : (1<<pidx);
        if ((wmask & pp->port_mask) == wmask) {
            *(pp->rpin) = wmask;
        }
        rc = PM_SUCCESS;
    }
    return rc;
}
