/****************************************************************************
 * loopback_driver.c
 * TESTING SUPPORT - MOCK Loopback character driver.
 * 
 * Creates a mock driver, for test purposes. Based on the avrlib
 * Driver stack API (driver.h, chardev.h) Ver 1.0
 *
 * Version 1.0
 *
 * IMPORTANT - This driver expects to be able to dynamically allocate
 *             memory. This may not be possible to use on an AVR target.
 * 
 * MANDITORY COMPILER DEFINES
 * 
 *   LOOPBACK_DRIVER  			enables the driver and allows it's 
 *                              registration
 * 
 * OPTIONAL COMPILER DEFINES
 * 
 *   MAX_LOOPBACK_INSTANCES 	set the number of minor loopback devices
 *   IN_BUFFER_ALLOC_SZ         set the size of each minor device Rx FIFO
 *   OUT_BUFFER_ALLOC_SZ		set the size of each minor device Tx FIFO
 ***************************************************************************/

#ifdef LOOPBACK_DRIVER

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avrlib/driver.h>


//#define TDD_PRINTF -- define in Makefile. For use by ATDD only!

#ifndef MAX_LOOPBACK_INSTANCES
  #define MAX_LOOPBACK_INSTANCES  8           /* max. number of allowed minor devices */
#endif
#ifndef IN_BUFFER_ALLOC_SZ
  #define IN_BUFFER_ALLOC_SZ      1024
#endif
#ifndef OUT_BUFFER_ALLOC_SZ
  #define OUT_BUFFER_ALLOC_SZ     1024
#endif

/* Driver Private Data - Per Instance --------------------------------*/

typedef struct loopback_data_type {
    int dev_handle;         // globally assigned device handle, 1+ AKA "file handle"
    int inst_index;         // The instance number. more than one can be open at the same time, if needed.
    char * in_buffer;       // MOCK incoming char data stream. See test usage, below
    char * out_buffer;      // MOCK outgoining char data stream. See test usage, below
    int in_buflen;          // size of 'in_buffer'
    int out_buflen;         // size of 'out_buffer'
    int in_head;            // start of the incoming data. where user code will next READ from
    int in_tail;            // end of incoming data. next location to write to.
    int out_head;           // User outgoing data starts here. Mock to start reading from here.
    int out_tail;           // last char written out by user. next location to write to.
    int in_len;             // current character count inside 'in_buffer'
    int out_len;            // current character count inside 'out_buffer'
} loopback_data_t;

// MOCK USAGE : 
//  MOCK [tail] --> in_buffer  --> user [head]  "Mock Input"
//  MOCK [head] <-- out_buffer <-- user [tail]  "Mock Output"
// -
// Mock Input:
//      call mock_loop_write to add mock data to the driver's input side.
// Mock Output:
//      call mock_loop_read to pull user data from the driver.


/* Driver Private Data - Global Context ------------------------------*/

typedef struct loopback_context_type {
	// MINOR DEVICE INFO:
    //   newly open loopback instances are stashed here. array indexing
    //   directs new instances by forming the index into the list.
    //   eg. user opens "/dev/loop/1" so instance saved into instlist[1]
    //       generates error if there is already an entry in index 1.
    //       list starts out all pre-filled with NULLS
    //   IMPORTANT - data is malloc'd and so must be free'd when closed.
    //   IMPORTANT - indexed by MINOR number, *NOT* file handle!
    loopback_data_t * instlist[MAX_LOOPBACK_INSTANCES];

} loopback_ctx_t;

static loopback_ctx_t _loopback_ctx;

static const char driver_name_prefix[] = "/dev/loop/";

static loopback_data_t * s_find_minor_ctx_by_minor_num( int minor ) {
	// simple indexing lookup...
	loopback_data_t * ctx = NULL;
	if ( minor >= 0 && minor < MAX_LOOPBACK_INSTANCES ) {
		ctx = _loopback_ctx.instlist[minor];
	}
	return ctx; // no guarantees it actually contains anything...
}

static loopback_data_t * s_find_minor_ctx_by_filehandle( int hndl ) {
	// find an entry with a matching file handle (device handle)
	loopback_data_t * ctx = NULL;
	int iter;
	for ( iter = 0 ; iter < MAX_LOOPBACK_INSTANCES ; ++iter ) {
		if ( _loopback_ctx.instlist[iter] && _loopback_ctx.instlist[iter]->dev_handle == hndl) {
			ctx = _loopback_ctx.instlist[iter];
			break;
		}
	}
	return ctx;
}

static int s_find_minor_ctxidx_by_filehandle( int hndl ) {
	// find an entry with a matching file handle (device handle)
	int cidx = -1;
	int iter;
	for ( iter = 0 ; iter < MAX_LOOPBACK_INSTANCES ; ++iter ) {
		if ( _loopback_ctx.instlist[iter] && _loopback_ctx.instlist[iter]->dev_handle == hndl) {
			cidx = iter;
			break;
		}
	}
	return cidx;
}

/* (!) Used only by loopback testing functions! */
/*
static int s_find_minornum_by_filehandle( int hndl ) {
	loopback_data_t * ctx = s_find_minor_ctx_by_filehandle(hndl);
	return (ctx) ? ctx->inst_index : -1;
}
*/

/* Driver Methods ----------------------------------------------------*/

// init( void ) --> void
static void loopdev_init(void) {
    int iter = 0;
#ifdef TDD_PRINTF
    printf("{loopdev_init}\n");
#endif    
    for (iter = 0 ; iter < MAX_LOOPBACK_INSTANCES ; ++iter) {
        _loopback_ctx.instlist[iter] = NULL; // invalid (not open) (!) malloc'd
    }
}


// open( (const char *)name, (const int) mode ) --> file_handle | STATUS
static int loopdev_open(const char * name, const int mode) {
    int rc = -1;
    // drop the prefix and look at the minor number
#ifdef TDD_PRINTF
    printf("{loopdev_open}\n");
#endif    
    if (name) {
        name += strlen(driver_name_prefix);
        int minor = (int)strtol(name,NULL,10);
#ifdef TDD_PRINTF
        printf("{loopdev_open} minor[%d] ...\n", minor);
#endif    
        if ((minor >= 0) && (minor < MAX_LOOPBACK_INSTANCES) && (_loopback_ctx.instlist[minor] == NULL)) {
            // create a new device for this minor number
            loopback_data_t * loopinst = (loopback_data_t *)malloc( sizeof(loopback_data_t) );
            if (loopinst) {
                loopinst->in_buffer  = (char *)malloc(IN_BUFFER_ALLOC_SZ);
                loopinst->out_buffer = (char *)malloc(OUT_BUFFER_ALLOC_SZ);
                if (loopinst->in_buffer && loopinst->out_buffer) {
                    // get a global device handle
                    loopinst->dev_handle = Driver_getHandle();
                    loopinst->inst_index = minor;
                    loopinst->in_buflen  = IN_BUFFER_ALLOC_SZ;
                    loopinst->out_buflen = OUT_BUFFER_ALLOC_SZ;
                    loopinst->in_head    = 0;
                    loopinst->in_tail    = 0;
                    loopinst->out_head   = 0;
                    loopinst->out_tail   = 0;
                    loopinst->in_len     = 0;
                    loopinst->out_len    = 0;
                    _loopback_ctx.instlist[minor] = loopinst;
                    rc = loopinst->dev_handle; // AKA "file handle"
#ifdef TDD_PRINTF
                    printf("{loopdev_open} minor[%d] OPENED for handle[%d]\n", minor, rc);
#endif    
                }
            }
        }
    }
    return rc;
}


// close( (void *) opctx ) --> STATUS
static int loopdev_close(int hndl) {
    int rc = -1;
    int cidx = s_find_minor_ctxidx_by_filehandle(hndl); 
	loopback_data_t * ctx = (cidx >= 0) 
		? _loopback_ctx.instlist[cidx] 
		: NULL;
#ifdef TDD_PRINTF
    printf("{loopdev_close} handle[%d] ctx_idx[%d]\n", hndl, cidx);
#endif
	if (ctx) {
#ifdef TDD_PRINTF
		int minst = ctx->inst_index;
#endif
		free( ctx->in_buffer );
		free( ctx->out_buffer );
		free( _loopback_ctx.instlist[cidx] );
		_loopback_ctx.instlist[cidx] = NULL;
#ifdef TDD_PRINTF
		printf("{loopdev_close} deleted minor device instance [%d] from the instance list\n",minst);
#endif
		rc = 0;
	}
#ifdef TDD_PRINTF
	else {
		printf("{loopdev_close} Error, minor device not found for given handle!\n");
	}
#endif    
    return rc;
}


// read( (void *) opctx, (char *)buffer, (int)len ) --> readcount | STATUS
static int loopdev_read(int hndl, char * buf, int maxlen) {
    int rc = -1;
    loopback_data_t * inst = s_find_minor_ctx_by_filehandle(hndl); 
#ifdef TDD_PRINTF
    printf("{loopdev_read} handle[%d] max-read[%d]\n", hndl, maxlen);
#endif    
    if (inst) {
        rc = 0;
        while (maxlen && (inst->in_len > 0)  ) {
            *(buf ++) = inst->in_buffer[ inst->in_head ];
            if (++(inst->in_head) >= inst->in_buflen) {
                inst->in_head = 0;
            }
            maxlen --;
            inst->in_len --;
            rc ++;
        }
    }
#ifdef TDD_PRINTF
	else {
		printf("{loopdev_read} Error, minor device not found for given handle!\n");
	}
	if (rc >= 0) {
		printf("{loopdev_read} handle[%d] minor[%d] read[%d]\n", hndl, inst->inst_index, rc);
	}
#endif    
    return rc;
}


// write( (void *) opctx, (char *)buffer, (int)len ) --> writecount | STATUS
static int loopdev_write(int hndl, const char * buf, int len) {
    int rc = -1;
    loopback_data_t * inst = s_find_minor_ctx_by_filehandle(hndl); 
#ifdef TDD_PRINTF
    printf("{loopdev_write} handle[%d] max-write[%d]\n", hndl, len);
#endif    
    if (inst) {
        rc = 0;
        while (len && (inst->out_len < inst->out_buflen)) {
            inst->out_buffer[ inst->out_tail ] = *(buf ++);
            if (++(inst->out_tail) >= inst->out_buflen) {
                inst->out_tail = 0;
            }
            len --;
            inst->out_len ++;
            rc ++;
        }
    }
#ifdef TDD_PRINTF
	else {
		printf("{loopdev_write} Error, minor device not found for given handle!\n");
	}
	if (rc >= 0) {
		printf("{loopdev_write} handle[%d] minor[%d] wrote[%d]\n", hndl, inst->inst_index, rc);
	}
#endif    
    return rc;
}


// reset( (void *) opctx, ) --> STATUS
static int loopdev_reset(int hndl) {
    int rc = -1;
    loopback_data_t * inst = s_find_minor_ctx_by_filehandle(hndl); 
#ifdef TDD_PRINTF
    printf("{loopdev_reset} handle[%d]\n", hndl);
#endif    
    if (inst) {
        inst->in_head = inst->in_tail = inst->in_len = 0;
        inst->out_head = inst->out_tail = inst->out_len = 0;
        rc = 0;
    }
#ifdef TDD_PRINTF
	else {
		printf("{loopdev_reset} Error, minor device not found for given handle!\n");
	}
#endif    
    return rc;
}


// Loopback device has no I/O control commands
// ioctl( (void *) opctx, (int)cmd, (int *)val ) --> STATUS
static int loopdev_ioctl(int hndl, int cmd, int * val) {
    int rc = -1;
    loopback_data_t * inst = s_find_minor_ctx_by_filehandle(hndl); 
#ifdef TDD_PRINTF
	if (val)
		printf("{loopdev_ioctl} handle[%d] cmd[%d] val[%d]\n", hndl, cmd, *val);
	else
		printf("{loopdev_ioctl} handle[%d] cmd[%d] val[<NULL>]\n", hndl, cmd);
#endif    
    if (inst && val) {
		switch (cmd) {
        case CMD_RX_PEEK:   // (cmd) --> (int)n   returns # bytes waiting in Rx buffer
			*val = inst->in_len;
			rc = 0;
			break;
        
        case CMD_TX_PEEK:   // (cmd) --> (int)n   returns # bytes of space available in Tx buffer
			*val = inst->out_len;
			rc = 0;
			break;
		
		default:
			rc = -1;
		}
    }
#ifdef TDD_PRINTF
	else {
		printf("{loopdev_ioctl} Error, minor device not found for given handle?\n");
	}
#endif    
    return rc;
}


/* Driver OBJECT (Parent) --------------------------------------------*/

static hdriver_t _loopback_inst = {
    loopdev_init,               // init     (global)
    loopdev_open,               // open     (global)
    loopdev_close,              // close    (ctx)
    loopdev_read,               // read     (ctx)
    loopdev_write,              // write    (ctx)
    loopdev_reset,              // reset    (ctx)
    loopdev_ioctl,              // ioctl    (ctx)
    (void *)&_loopback_ctx      // driver-class opaque data     (global)
};

/* Driver Registration - Called by main() ----------------------------*/

int loopback_Register(void) {
#ifdef TDD_PRINTF
    printf("{loopback_Register}\n");
#endif    
    return Driver_registerClass(driver_name_prefix, &_loopback_inst);
}

/* Mock Hooks - Backend I/O Operations -------------------------------*/

// check to see if a mock instance has been setup. 
// (!) "instance" is the driver's open MINOR number device! It is *NOT* the file handle.
// Return: treat as a bool. True := instance established.
int mock_loop_check_instance(int instance) {
#ifdef TDD_PRINTF
    printf("{mock_loop_check_instance} inst[%d]\n", instance);
#endif
	loopback_data_t * inst = s_find_minor_ctx_by_minor_num(instance);
    return (inst) ? 1 : 0;
}

int mock_loop_write(int instance, const char * buf, int len) {
    int rc = -1;
    loopback_data_t * inst = s_find_minor_ctx_by_minor_num(instance);
#ifdef TDD_PRINTF
    printf("{mock_loop_write} inst[%d] max-write[%d]\n", instance, len);
#endif    
    if (inst) {
        rc = 0;
        while (len && (inst->in_len < inst->in_buflen)) {
            inst->in_buffer[ inst->in_tail ] = *(buf ++);
            if (++inst->in_tail >= inst->in_buflen) {
                inst->in_tail = 0;
            }
            len --;
            inst->in_len ++;
            rc ++;
        }
    }
    return rc;
}

int mock_loop_read(int instance, char * buf, int maxlen) {
    int rc = -1;
    loopback_data_t * inst = s_find_minor_ctx_by_minor_num(instance);
#ifdef TDD_PRINTF
    printf("{mock_loop_read} inst[%d] max-read[%d]\n", instance, maxlen);
#endif    
    if (inst) {
        loopback_data_t * inst = _loopback_ctx.instlist[instance];
        rc = 0;
        while (maxlen && (inst->out_len > 0)  ) {
            *(buf ++) = inst->out_buffer[ inst->out_head ];
            if (++inst->out_head >= inst->out_buflen) {
                inst->out_head = 0;
            }
            maxlen --;
            inst->out_len --;
            rc ++;
        }
    }
    return rc;
}

int mock_loop_reset(int instance) {
	int rc = -1;
	loopback_data_t * inst = s_find_minor_ctx_by_minor_num(instance);
	if (inst) {
		rc = loopdev_reset(inst->dev_handle);
	}
	return rc;
}

#endif /* LOOPBACK_DRIVER */
