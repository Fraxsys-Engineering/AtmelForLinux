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
 * STRING LIMITS
 * 
 * [1] Maximum length of a device "name" descriptor, with all arguments
 *     Set MAX_DEVSTRN_LEN as-needed. If not defined at compile time then
 *     a maximum limit of 32 characters is established.
 ***************************************************************************/

/* STATUS: Written, not compiled */

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include "ioctlcmds.h"

#if 0
// Atmel Internal register pointer types
typedef volatile uint8_t *  sfr8p_t;
typedef volatile uint16_t * sfr16p_t;
typedef volatile uint32_t * sfr32p_t;
#endif

// Maximum limits - needed for creating static length string buffers, etc.
#ifndef MAX_DEVSTRN_LEN
#define MAX_DEVSTRN_LEN  32
#endif

// init( void ) --> void
typedef void (*f_init)( void );

// open( (const char *)name, (const int) mode ) --> file_handle | STATUS
typedef int (*f_open)(const char *, const int);

// close( (void *) opctx ) --> STATUS
typedef int (*f_close)(void *);

// read( (void *) opctx, (char *)buffer, (int)len ) --> readcount | STATUS
typedef int (*f_read)(void *, char *, int );

// write( (void *) opctx, (char *)buffer, (int)len ) --> writecount | STATUS
typedef int (*f_write)(void *, char *, int );

// reset( (void *) opctx, ) --> STATUS
typedef int (*f_reset)(void *);

// ioctl( (void *) opctx, (int)cmd, (int *)val ) --> STATUS
typedef int (*f_ioctl)(void *, int, int * );

typedef struct driverhandle_type {
    f_init    init;     // init     (global)
    f_open    open;     // open     (global)
    f_close   close;    // close    (ctx)
    f_read    read;     // read     (ctx)
    f_write   write;    // write    (ctx)
    f_reset   reset;    // reset    (ctx)
    f_ioctl   ioctl;    // ioctl    (ctx)
    int       fhandle;  // handle, idx to context table (global, set to zero)
    void *    opdrvr;   // driver-class opaque data     (global) (*1)
    void *    opctx;    // driver-instance opaque data  (ctx)    (*2)
} hdriver_t;
// Notes
// *1   - global driver has this pointer setup in it's instance and it
//        points to the single global data structure for managing all
//        active instances of the same driver type.
// *2   - global driver object instance has this set to NULL. When 
//        copied, the parameter will be updated to reflact the instance
//        being used in the call.


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

// System Initialization - The user main loop can call a public API
// to init internal drivers. This function will get called and will go
// to each registered driver and call it's init() function.
extern int System_driverInit(void);

// Called by the public API, this function is used to obtain a handle to
// a driver instance for usage. The handle is returned by an instance
// call, there is no need for a global function.
// Returns: driver_handle, or -1 on error.
extern int System_driverInstance(const char * name, int mode);

// Called by chardev code, this function returns the driver handle to 
// an open driver instance. Returns NULL if the given file_handle is
// invalid.
// USAGE: Caller creates a struct of type hdriver_t on it's stack. The
//        pointer to this memory is passed in ('instmem') and driver 
//        info is copied into it. The pointer is also returned and may be 
//        NULL on error.
//        Once the caller function leaves its scope, the local copied
//        driver info is lost. Further calls will repeat the process.
extern hdriver_t * CharDev_Get_Instance(int driver_handle, hdriver_t * instmem);

#endif /* _DRIVER_H_ */
