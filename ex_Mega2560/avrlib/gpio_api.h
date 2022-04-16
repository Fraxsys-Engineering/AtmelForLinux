/**********************************************************************
 * gpio_api.h
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

#ifndef _GPIO_API_H_
#define _GPIO_API_H_

#include "avrlib.h"

#ifdef EMULATE_LIB
 #define TEST_PORT_COUNT  2  /* PortA, PortB */
 #define TEST_REGR_COUNT  (TEST_PORT_COUNT*3)
#endif

/* Generic Standard Return Codes ------------------------------------*/
#define PM_SUCCESS          0
#define PM_ERROR            (-1)

/* Global Pullup Control --------------------------------------------*/
#define PM_PUP_ENABLED      0       /* pull-ups work normally */
#define PM_PUP_DISABLED     1       /* all pull-ups disabled */

/* GPIO *PIN* or *PORT* - Set initial direction and state -----------*/
#define PINMODE_UNCHANGED   0
#define PINMODE_INPUT_TRI   1
#define PINMODE_INPUT_PU    2
#define PINMODE_OUTPUT_LO   3
#define PINMODE_OUTPUT_HI   4

#define PINMODE_TOTALMODES  5

/* GPIO - REGISTER A PIN OR A COMPLETE PORT (8 bits) ----------------*/
#define PM_REG_PIN          0
#define PM_REG_PORT         1

/* Available Ports --------------------------------------------------
 * -
 * Actual available Ports will be determined in the source code and 
 * attempting to access a non-existant port on the selected MCU target
 * will throw an error.
 * ------------------------------------------------------------------*/
#define PM_PORT_A           0
#define PM_PORT_B           1
#define PM_PORT_C           2
#define PM_PORT_D           3
#define PM_PORT_E           4
#define PM_PORT_F           5
#define PM_PORT_G           6       /* 6 pins */
#define PM_PORT_H           7
#define PM_PORT_J           8
#define PM_PORT_K           9
#define PM_PORT_L           10

#define PORT_COUNT          11

/* Available Pins ---------------------------------------------------
 * -
 * Pincount can vary per port. If a non existant pin is requested on
 * a port then an error is thrown.
 * ------------------------------------------------------------------*/
#define PM_PIN_0            0
#define PM_PIN_1            1
#define PM_PIN_2            2
#define PM_PIN_3            3
#define PM_PIN_4            4
#define PM_PIN_5            5
#define PM_PIN_6            6
#define PM_PIN_7            7

#define PM_TOTALPINS        8

/* CALLED BY SYSTEM INIT or MAIN LOOP ONLY - Initialize Gpio --------
 * -
 * Call this once from the main loop or other system initialization 
 * routine.
 * ------------------------------------------------------------------*/
void pm_init(void);

/* Global Pull-Up Control -------------------------------------------
 * -
 * Pullups on all input pins w/ selected pullups, can be globally 
 * enabled or disabled by calling this method. By default, global
 * pullup control is enabled and all pullups will work.
 * -
 * Arguments:
 *  pupctrl     one of: PM_PUP_ENABLED, PM_PUP_DISABLED
 * Returns:     PM_SUCCESS, PM_ERROR
 * ------------------------------------------------------------------*/
int pm_glb_pup_control(uint8_t pupctrl);

/* Registration API -------------------------------------------------
 * -
 * Register a GPIO pin or port (prt) for usage. Once registered, other
 * attempts to register the same pin, or the entire port which this 
 * pin lies in, will return -1 (PM_ERROR).
 * -
 * On success, the call will return a "handle" which is used in 
 * subsequent gpio calls to control the pin. The handle is a positive
 * non-zero value.
 * -
 * Note: A return of zero is reserved by other calls as a PM_SUCCESS
 *       code and so a handle will never be zero.
 * Note: When setting a port with pullup/input mode, *ALL* pins will 
 *       be set to use a pullup.
 * -
 * Arguments:
 *  port        one of: PM_PORT_*
 *  pin         (pin) one of: PM_PIN_*
 *  byt         (prt) (out): pre-set port output, (in): ignored.
 *  mode        one of: PINMODE_***_**
 * ------------------------------------------------------------------*/
int pm_register_pin(uint8_t port, uint8_t pin, uint8_t mode);
int pm_register_prt(uint8_t port, uint8_t byt, uint8_t mode);

/* Change Pin, Port Direction ---------------------------------------
 * -
 * If a port, then all pins change to the same direction.
 * -
 * Arguments:
 *  hndl        registration handle
 *  mode        one of: PINMODE_***_**
 * Returns:     PM_SUCCESS, PM_ERROR
 * ------------------------------------------------------------------*/
int pm_chg_dir(int hndl, uint8_t mode);

/* Change Pin, Port State -------------------------------------------
 * -
 * For pins, a 0 will clear and any non-zero value will set the PIN
 * high. For PORTS, enter a complete byte value to affect all 8 pins.
 * Ports which use a sub-set of less than 8 pins will ignore the bit
 * position in the given value.
 * -
 * Arguments:
 *  hndl        registration handle
 *  value       PIN {0,1}, PORT {0x00 - 0xFF}
 * Returns:     PM_SUCCESS, PM_ERROR
 * ------------------------------------------------------------------*/
int pm_out(int hndl, uint8_t value);

/* Read a Pin or Port -----------------------------------------------
 * -
 * Return a value corresponding to the pin or ports external state.
 * Pins, ports should be set as inputs if reading the external voltage
 * state on the correcponding MCU pin and will be affected also by the
 * internal pullup resistor state.
 * -
 * Pins:  Return (value) will be '0' (logic low) or '1' (logic high)
 * Ports: Return (value) will be {0x00 - 0xFF} with appropriate 
 *        bit-masking of unused port pins to state:0. Eg a 6-pin port
 *        (b0 .. b5) can only return a maximum value of 0x3F.
 * -
 * Arguments:
 *  hndl        registration handle
 * Returns:    PM_ERROR, (value)
 * ------------------------------------------------------------------*/
int pm_in(int hndl);

/* PIN ONLY, Toggle Pinout State ------------------------------------
 * -
 * Only affects single bit GPIO pins, set to OUTPUT.
 * Toggle the output state, 0 --> 1, 1 --> 0
 * -
 * Arguments:
 *  hndl        registration handle
 * Returns:     PM_SUCCESS, PM_ERROR
 * ------------------------------------------------------------------*/
int pm_tog(int hndl);

#endif /* _GPIO_API_H_ */
