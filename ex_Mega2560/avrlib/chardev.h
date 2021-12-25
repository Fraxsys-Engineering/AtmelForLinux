/****************************************************************************
 * chardev.h
 * USER API and I/O structure of a character driver.
 * 
 * This header is for public "user" usage.
 * 
 * Version 1.0
 *
 * BASIC FUNCTIONS
 *  cd_open()	Open a device instance by name
 *  cd_close()  Close an open device.
 *  cd_read()   Character stream read (byte buffer)
 *  cd_write()  Character stream write (byte buffer)
 *  cd_reset()  Reset the open driver, if applicable.
 *  cd_ioctl()  Misc. other functions and out-of-band operations.
 * 
 * File Handles - Valid handles are positive integers greater than zero.
 * 
 * NAMESPACES - All Devices
 * 
 * SERIAL UART {format} : /dev/uart/<driver-instance>[,options]
 *       <driver-instance> : {0,1,2,3}  {2,3} are not available on all devices.
 *                 Options : ,baud,bits,parity,stops
 *                 baud   := {9600, 19200, 38400, 57600, 115200}
 *                 bits   := {5,6,7,8}        
 *                 parity := {N,E,O} N[one], E[ven], O[dd]
 *                 stops  := {1,2}
 * 
 * LOOPBACK (Mock Testing) {format} : /dev/loop/<minor-num>
 *      <minor-num> : {0 .. MAX_LOOPBACK_INSTANCES} see implimentation code.
 *      (!) IMPORTANT - This driver is used for testing only and may not
 *                      be available on AVR targets. Refer to testing code
 *                      in the avrlinuxtest/ directories.
 ***************************************************************************/

/* STATUS: Written, not compiled */

#ifndef _CHARDEV_H_
#define _CHARDEV_H_

#include "ioctlcmds.h"

/**********************************************************************
 * OPEN
 * Open a new driver instance and return a file handle reference.
 * int open(const char * name, int mode)
 *   'name'         [in]    device name and options, see docs in header.
 *   'mode'         [in]    optional mode. Set zero if not used.
 *   Returns:               file_handle (1+) | DEV_FAIL
 *********************************************************************/
extern int cd_open(const char * name, int mode);

/**********************************************************************
 * CLOSE
 *    int ioctl(int file_handle)
 *   'file_handle'  [in]    Open handle
 *   Returns:               DEV_SUCCESS | DEV_FAIL
 *********************************************************************/
extern int cd_close(int file_handle);

/**********************************************************************
 * READ
 *    int ioctl(int file_handle, char * buf, int len)
 *   'file_handle'  [in]    Open handle
 *   'buf'          [in]    buffer for copying data into
 *   'len'          [in]    maximum # bytes that can be copied into 'buf'
 *   Returns:               # chars copied (0+) | DEV_FAIL
 *********************************************************************/
extern int cd_read(int file_handle, char * buf, int len);

/**********************************************************************
 * WRITE
 *    int ioctl(int file_handle, char * buf, int len)
 *   'file_handle'  [in]    Open handle
 *   'buf'          [in]    buffer for copying data out of
 *   'len'          [in]    # bytes to copy out of 'buf'
 *   Returns:               # chars copied (0+) | DEV_FAIL
 *********************************************************************/
extern int cd_write(int file_handle, char * buf, int len);

/**********************************************************************
 * IOCTL
 *    int ioctl(int file_handle, int cmd, int * val)
 *   'file_handle'  [in]
 *   'cmd'          [in]    the command (enum) specific to the driver class
 *   'val'      [in,out]    the dataset for the command, int or mem-pointer 
 *                          to something else
 *   Returns:               0 on success, -1 on failure.
 *********************************************************************/
extern int cd_ioctl(int file_handle, int cmd, int * val);

#endif /* _CHARDEV_H_ */
