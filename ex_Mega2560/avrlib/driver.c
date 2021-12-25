/****************************************************************************
 * driver.c
 * Internal code for supporting character device drivers
 * 
 * This implimentation is for driver usage and not exposed out to 
 * "user" code.
 * User API: chardev.h
 * 
 * Version 1.0
 *
 ***************************************************************************/

/* STATUS: write incomplete (last fcn) */

#include "driver.h"
#include <string.h>

//#define TDD_PRINTF -- define in Makefile. For use by ATDD only!

#ifdef TDD_PRINTF
#include <stdio.h>
#endif

#ifndef MAX_CHAR_DRIVERS
#define MAX_CHAR_DRIVERS  2
#endif

typedef struct chardev_reg_type {
    const char *         ns_proto;  // eg. "/dev/uart/"
    const hdriver_t *     driver;
} chardev_reg_t;

static chardev_reg_t chardev_registry[MAX_CHAR_DRIVERS] = {0};

// local working string buffer - not reent safe!
//static char name_buffer[MAX_DEVSTRN_LEN];

// GLOBAL active device instance number (1+). Zero not valid.
static int last_instance = 0;

// Register Driver Class - Called once by each generic character driver
// type eg. serialdriver. This registers the class with a namespace
// prototype that can be used to find appropriate drivers by name. The
// registered driver's open() function is called to decode the namepace
// options, open the underlying channel, register the new instance and
// return a file handle number.
int Driver_registerClass(const char * ns_proto, hdriver_t * stack) {
    int iter = 0;
    int rc = -1;
#ifdef TDD_PRINTF
	printf("{Driver_registerClass}\n");
#endif
    for ( iter = 0; iter < MAX_CHAR_DRIVERS; ++iter ) {
        if (chardev_registry[iter].driver == NULL) {
            chardev_registry[iter].ns_proto = ns_proto;
            chardev_registry[iter].driver = stack;
            rc = iter;
            break;
        } 
    }
    return rc;
}

// Get a new globally unique file handle.
int Driver_getHandle(void) {
    last_instance ++;
#ifdef TDD_PRINTF
	printf("{Driver_getHandle} New handle[%d]\n", last_instance);
#endif
    return last_instance;
}

// System Initialization - The user main loop can call a public API
// to init internal drivers. This function will get called and will go
// to each registered driver and call it's init() function.
int System_driverInit(void) {
    int iter = 0;
    int rc = 0;
#ifdef TDD_PRINTF
	printf("{System_driverInit}\n");
#endif
    for ( iter = 0; iter < MAX_CHAR_DRIVERS; ++iter ) {
        if (chardev_registry[iter].driver) {
            chardev_registry[iter].driver->init();
            rc ++;
        } 
    }
    return rc;
}

// Called by the public API, this function is used to obtain a handle to
// a driver instance for usage. The handle is returned by an instance
// call, there is no need for a global function.
// Returns: driver_handle, or -1 on error.
int System_driverInstance(const char * name, int mode) {
    int iter = 0;
    int rc = -1;
#ifdef TDD_PRINTF
	printf("{System_driverInstance} name[%s] mode[%d]\n", name, mode);
#endif
    for ( iter = 0; iter < MAX_CHAR_DRIVERS; ++iter ) {
        if (name && chardev_registry[iter].ns_proto && chardev_registry[iter].driver) {
            if (strncmp(chardev_registry[iter].ns_proto, name, strlen(chardev_registry[iter].ns_proto)) == 0) {
                rc = chardev_registry[iter].driver->open(name,mode);
                break;
            }
        }
    }
#ifdef TDD_PRINTF
	printf("{System_driverInstance} rc[%d]\n", rc);
#endif
    return rc;
}

// Called by chardev code, this function returns the driver handle to 
// an open driver instance. Returns NULL if the given file_handle is
// invalid.
// USAGE: Caller creates a struct of type hdriver_t on it's stack. The
//        pointer to this memory is passed in ('instmem') and driver 
//        info is copied into it. The pointer is also returned and may be 
//        NULL on error.
//        Once the caller function leaves its scope, the local copied
//        driver info is lost. Further calls will repeat the process.
hdriver_t * CharDev_Get_Instance(int driver_handle, hdriver_t * instmem) {
#ifdef TDD_PRINTF
	printf("{CharDev_Get_Instance} -FIXME- -REFACTOR- handle[%d]\n", driver_handle);
#endif
	// Dec 23 2021 ---- Needs refactoring ----------------------------
	// TODO - one particular driver, eg. usart, may have multiple minor
	//        instances open and this code + driver "parent" needs to
	//        get the handle to the correct child instance for any 
	//        operation (read,write,ioctl,close).
	// Problem: The driver registration table 'chardev_registry' only
	//          contains the parent drivers. How-to get the open 
	//          instance handle down to the corrent driver child?
	// Problem: The parent driver struct 'hdriver_t' contains a file
	//          handle variable... why? parent could be called with
	//          several handles, one for each open child instance.
	return NULL; // FIXME!
}

