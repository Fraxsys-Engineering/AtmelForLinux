/****************************************************************************
 * driver.h
 * INTERNAL API and I/O structure of a character driver.
 * 
 * This header is for driver usage and not exposed out to "user" code.
 * User API: chardev.h
 * 
 * Version 1.0
 *
 * BASIC FUNCTIONS
 *  init()      Pre-start operations. called once
 *  open()      Open a device instance by name
 *  close()     Close an open device.
 *  read()      Character stream read (byte buffer)
 *  write()     Character stream write (byte buffer)
 *  reset()     Reset the open driver, if applicable.
 *  ioctl()     Misc. other functions and out-of-band operations.
 *
 * DRIVER NAMESPACE
 * [1.0] Generic format
 *        /dev/<driver-class>/<driver-instance>[,options]
 *
 * [2.0] Serial Drivers
 *         /dev/uart/{0..n}[,options]
 *  [2.1] Options
 *        ,baud,bits,parity,stops
 *            baud   := {9600, 19200, 38400, 57600, 115200}
 *            bits   := {5,6,7,8}        
 *            parity := {N,E,O} N[one], E[ven], O[dd]
 *            stops  := {1,2}
 *  [2.2] IOCTL Commands
 *        CMD_RX_PEEK --> (int)n        returns # bytes waiting in Rx buffer
 *        CMD_TX_PEEK --> (int)n        returns # bytes of space available in 
 *                                    the Tx buffer
 *        CMD_RD_BAUD --> (int)n      returns the actual baudrate being used
 *
 * [3.0] *future* - SPI, I2C, files
 * 
 * IOCTL
 *    int ioctl( (int)cmd, (int*)val )
 *        'cmd' [in]        the command (enum) specific to the driver class
 *        'val' [in,out]    the dataset for the command, int or mem-pointer 
 *                        to something else
 *        Returns:        0 on success, -1 on failure.
 * 
 * LIMITS
 * Limits can be over-ridden at compile time using an external define
 * parameter (-D).
 * 
 * [1] MAX_DEVSTRN_LEN
 *     Maximum length of a device "name" descriptor, with all arguments
 *     Set MAX_DEVSTRN_LEN as-needed. Default: 32 characters.
 * 
 * [2] MAX_OPEN_DESCRIPTORS
 *     Maximum number of descriptors that can be opened at any point in
 *     time. Default is 8.
 * 
 * [3] MAX_CHAR_DRIVERS
 *     (This should be internally adjusted as more drivers are added to
 *      the library)
 *     Define the maximum number of char stream driver types that can 
 *     be registered for use by user applications.
 * 
 ***************************************************************************/

/* STATUS: Written, not compiled */

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <stdint.h>
#include "ioctlcmds.h"

// Atmel Internal register pointer types
typedef volatile uint8_t *  sfr8p_t;
typedef volatile uint16_t * sfr16p_t;
typedef volatile uint32_t * sfr32p_t;

// Maximum limits - needed for creating static length string buffers, etc.
#ifndef MAX_DEVSTRN_LEN
#define MAX_DEVSTRN_LEN  32
#endif

// Maximum number of allowed simultaneously open file descriptors.
// needed to create a static descriptor registration table.
#ifndef MAX_OPEN_DESCRIPTORS
#define MAX_OPEN_DESCRIPTORS 8
#endif

// Maximum number of driver types that can be registered. Expand this as
// more char stream drivers are added to the library.
#ifndef MAX_CHAR_DRIVERS
#define MAX_CHAR_DRIVERS  2
#endif

// init( void ) --> void
typedef void (*f_init)( void );

// open( (const char *)name, (const int) mode ) --> file_handle | STATUS
typedef int (*f_open)(const char *, const int);

// close( (int) hndl ) --> STATUS
typedef int (*f_close)(int);

// read( (int) hndl, (char *)buffer, (int)len ) --> readcount | STATUS
typedef int (*f_read)(int, char *, int );

// write( (int) hndl, (char *)buffer, (int)len ) --> writecount | STATUS
typedef int (*f_write)(int, const char *, int );

// reset( (int) hndl ) --> STATUS
typedef int (*f_reset)(int);

// ioctl( (int) hndl, (int)cmd, (int *)val ) --> STATUS
typedef int (*f_ioctl)(int, int, int * );

typedef struct driverhandle_type {
    f_init    init;     // init     (global)
    f_open    open;     // open     (global)
    f_close   close;    // close    (ctx)
    f_read    read;     // read     (ctx)
    f_write   write;    // write    (ctx)
    f_reset   reset;    // reset    (ctx)
    f_ioctl   ioctl;    // ioctl    (ctx)
    void *    opdrvr;   // driver-class opaque data     (global) (*1)
} hdriver_t;
// Notes
// *1   - global driver has this pointer setup in it's instance and it
//        points to the single global data structure for managing all
//        active instances of the same driver type. Any info for minor
//        devices will be buried inside of this "global" driver data
//        and it is up to each driver to manage that.



// Driver stack intial setup. THIS MUST BE CALLED FIRST BEFORE ALL 
// OTHER CALLS
// Note: This also now registers all enabled drivers. Drivers are enabled
//        through specific define switches, see serialdriver.c for examples
//        eg. UART_ENABLE_PORT_0
extern void System_DriverStartup(void);


// Register Driver Class - Called once by each generic character driver
// type eg. serialdriver. This registers the class with a namespace
// prototype that can be used to find appropriate drivers by name. The
// registered driver's open() function is called to decode the namepace
// options, open the underlying channel, register the new instance and
// return a file handle number.
extern int Driver_registerClass(const char * ns_proto, hdriver_t * stack);


// Called by Driver's open() function, this allocates a new global file
// handle to the new instance.
extern int Driver_getHandle(void);


// Called by Driver's close() function, this returns a file handle, cleaning
// up any cached handle/driver hash table entries and freeing up this 
// handle slot for future usage.
extern void Driver_returnHandle(int driver_handle);


// Registered Driver Initialization - This function will call each 
// registered driver and initialize it. This would be called *AFTER* 
// System_DriverStartup() *AND* all individual driver registration calls.
extern int System_driverInit(void);


// Called by the public API, this function is used to obtain a handle to
// a driver instance for usage. The handle is returned by an instance
// call, there is no need for a global function.
// Returns: driver_handle, or -1 on error.
extern int System_driverInstance(const char * name, int mode);


// Called by chardev code, this function returns the driver handle to 
// an open driver instance (minor device). Returns NULL if the given 
// file_handle is invalid.
extern const hdriver_t * CharDev_Get_Instance(int driver_handle);


#endif /* _DRIVER_H_ */
