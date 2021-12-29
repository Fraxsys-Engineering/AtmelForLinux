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

typedef struct chardev_reg_type {
    const char *         ns_proto;  // eg. "/dev/uart/"
    const hdriver_t *    driver;
} chardev_reg_t;

static chardev_reg_t chardev_registry[MAX_CHAR_DRIVERS] = {0};

typedef struct chardev_fh_entry_type {
	int        	        file_handle;
	const hdriver_t *   driver;
} chardev_fh_t;

static chardev_fh_t open_filehandle_table[MAX_OPEN_DESCRIPTORS] = {0};
static int open_filehandle_count = 0;

// local working string buffer - not reent safe!
//static char name_buffer[MAX_DEVSTRN_LEN];

// GLOBAL active device instance number (1+). Zero not valid.
static int last_instance = 0;

// System Initialization - *** CALL FIRST! ***
void System_DriverStartup(void) {
	int iter;
	for ( iter = 0 ; iter < MAX_OPEN_DESCRIPTORS ; ++iter ) {
		open_filehandle_table[iter].file_handle = 0;
		open_filehandle_table[iter].driver = NULL;
	}
	open_filehandle_count = 0;
}

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
    if (++last_instance == 0)
		last_instance ++; // rollover - zero is an invalid handle ID
#ifdef TDD_PRINTF
	printf("{Driver_getHandle} New handle[%d]\n", last_instance);
#endif
    return last_instance;
}

// Return a file handle.
void Driver_returnHandle(int driver_handle) {
	int iter;
#ifdef TDD_PRINTF
	int fcount = 0;
	printf("{Driver_returnHandle} handle[%d]\n", driver_handle);
#endif
	for ( iter = 0 ; iter < MAX_OPEN_DESCRIPTORS ; ++iter ) {
		if (open_filehandle_table[iter].file_handle == driver_handle) {
			open_filehandle_table[iter].file_handle = 0;
			open_filehandle_table[iter].driver = NULL;
#ifdef TDD_PRINTF
			fcount ++;
#endif
		}
	}
	if (open_filehandle_count > 0) {
		open_filehandle_count --;
	}
#ifdef TDD_PRINTF
	else {
		printf("{Driver_returnHandle} SYSTEM BUG - open file handle count was already zero!\n");
	}
	if (fcount > 1) {
		printf("{Driver_returnHandle} SYSTEM BUG - found the same handle twice in the handle registration list!\n");
	}
#endif
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
    int jter = 0;
    int rc   = -1;
#ifdef TDD_PRINTF
	printf("{System_driverInstance} name[%s] mode[%d]\n", name, mode);
#endif
	// descriptor table full?
	if (open_filehandle_count < MAX_OPEN_DESCRIPTORS) {
		for ( iter = 0; iter < MAX_CHAR_DRIVERS; ++iter ) {
			if (name && chardev_registry[iter].ns_proto && chardev_registry[iter].driver) {
				if (strncmp(chardev_registry[iter].ns_proto, name, strlen(chardev_registry[iter].ns_proto)) == 0) {
					rc = chardev_registry[iter].driver->open(name,mode);
					// if handle is valid then register it in the global active
					// file handle table.
					if (rc > 0) {
						for ( jter = 0 ; jter < MAX_OPEN_DESCRIPTORS ; ++jter ) {
							if ( open_filehandle_table[jter].file_handle == 0 && open_filehandle_table[jter].driver == NULL ) {
								open_filehandle_table[jter].file_handle = rc;
								open_filehandle_table[jter].driver = chardev_registry[iter].driver;
								break;
							}
						}
						open_filehandle_count ++;
					}
					break;
				}
			}
		}
	}
#ifdef TDD_PRINTF
	else {
		printf("{System_driverInstance} OPEN FILEHANDLE TABLE FULL!\n");
	}
#endif
	



#ifdef TDD_PRINTF
	printf("{System_driverInstance} rc[%d]\n", rc);
#endif
    return rc;
}

// Called by chardev code, this function returns the driver handle to 
// an open driver instance. Returns NULL if the given file_handle is
// invalid.
// (Operations Notes)
// [1] Opened file handles are added to a global table of active handles.
//     this table also contains a pointer copy of the driver tasked with
//     managing the open handle; a minor device number of the generic
//     driver type.
//     Generic drivers handle all minor numbers within their scope so the
//     same driver pointer can be set to multiple active file handles at
//     the same time.
// --------------------------------------------------------------------
const hdriver_t * CharDev_Get_Instance(int driver_handle) {
	int iter = 0;
	const hdriver_t * drvr = NULL;
#ifdef TDD_PRINTF
	printf("{CharDev_Get_Instance} handle[%d] active handle count[%d]\n", driver_handle, open_filehandle_count);
#endif
	// All active and open file handles are registered in: open_filehandle_table[]
	if ( open_filehandle_count ) {
		for ( iter = 0 ; iter < MAX_OPEN_DESCRIPTORS ; ++iter ) {
			if ( open_filehandle_table[iter].file_handle == driver_handle ) {
				drvr = open_filehandle_table[iter].driver;
				break;
			}
		}
	}
	return drvr;
}

