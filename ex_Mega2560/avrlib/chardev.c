/****************************************************************************
 * chardev.c
 * Implimentation of a character driver.
 * 
 * Version 1.0
 *
 * Notes:
 * [1]    Underlying driver has one global static instance with all methods.
 *      The returned driver handle (dh) is a *copy* of the global instance
 *      but with the local context configured (normally is NULL). This 
 *      allows the global driver object to handle multiple instances
 *      without needing to stash duplicates of the driver object, one for
 *      each instance. The copy is on the stack only for the duration of 
 *      the chardev call, then it is discarded.
 * [2]  Above system could be speed optimized but all open driver instances
 *      would have to be saved in a memory list during their lifetime, 
 *      consuming memory while the instance is open. If there is no dynamic
 *      memory available then the total number of instances would have to 
 *      be boxed at an upper limit and all of that needed memory would have
 *         to be pre-allocated in RAM.
 * [3]  This layer serves as the shim layer between the user character
 *      device I/O methods (common to all char drivers) and the internal 
 *      hidden drivers themselves.
 ***************************************************************************/

/* STATUS: Written, not compiled */

#include "chardev.h"
#include "driver.h"

//#define TDD_PRINTF -- define in Makefile. For use by ATDD only!

#ifdef TDD_PRINTF
#include <stdio.h>
#endif

int cd_open(const char * name, int mode) {
#ifdef TDD_PRINTF
	printf("{cd_open} name[%s] mode[%d]\n", name, mode);
	int rc = System_driverInstance(name, mode);
	printf("{cd_open} rc[%d]\n", rc);
	return rc;
#else
	return System_driverInstance(name, mode);
#endif
}

int cd_close(int file_handle) {
#ifdef TDD_PRINTF
	printf("{cd_close} handle[%d]\n", file_handle);
#endif
    const hdriver_t * dh = CharDev_Get_Instance(file_handle);
    int rc = -1;
    if (dh) {
        rc = dh->close(file_handle);
        Driver_returnHandle(file_handle);
    }
#ifdef TDD_PRINTF
	else {
		printf("{cd_close} Error, CharDev_Get_Instance(%d) returned NULL!\n", file_handle);
	}
	printf("{cd_close} rc[%d]\n", rc);
#endif
    return rc;
}

int cd_read(int file_handle, char * buf, int len) {
#ifdef TDD_PRINTF
	printf("{cd_read} handle[%d]\n",file_handle);
#endif
    const hdriver_t * dh = CharDev_Get_Instance(file_handle);
    int rc = -1;
    if (dh) {
        rc = dh->read(file_handle, buf, len);
    }
#ifdef TDD_PRINTF
	else {
		printf("{cd_read} Error, CharDev_Get_Instance(%d) returned NULL!\n", file_handle);
	}
	printf("{cd_read} rc[%d]\n", rc);
#endif
    return rc;
}

int cd_write(int file_handle, const char * buf, int len) {
#ifdef TDD_PRINTF
	printf("{cd_write} handle[%d]\n",file_handle);
#endif
    const hdriver_t * dh = CharDev_Get_Instance(file_handle);
    int rc = -1;
    if (dh) {
        rc = dh->write(file_handle, buf, len);
    }
#ifdef TDD_PRINTF
	else {
		printf("{cd_write} Error, CharDev_Get_Instance(%d) returned NULL!\n", file_handle);
	}
	printf("{cd_write} rc[%d]\n", rc);
#endif
    return rc;
}

int cd_ioctl(int file_handle, int cmd, int * val) {
#ifdef TDD_PRINTF
	printf("{cd_ioctl} handle[%d] cmd[%d]\n", file_handle, cmd);
#endif
    const hdriver_t * dh = CharDev_Get_Instance(file_handle);
    int rc = -1;
    if (dh) {
        rc = dh->ioctl(file_handle, cmd, val);
    }
#ifdef TDD_PRINTF
	else {
		printf("{cd_ioctl} Error, CharDev_Get_Instance(%d) returned NULL!\n", file_handle);
	}
	printf("{cd_ioctl} rc[%d]\n", rc);
#endif
    return rc;
}
