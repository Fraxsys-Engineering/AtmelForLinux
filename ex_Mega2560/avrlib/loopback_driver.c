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
 ***************************************************************************/

/* STATUS: Written, not compiled */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avrlib/driver.h>


//#define TDD_PRINTF -- define in Makefile. For use by ATDD only!

#define MAX_LOOPBACK_INSTANCES  8           /* max. number of allowed minor devices */
#define IN_BUFFER_ALLOC_SZ      1024
#define OUT_BUFFER_ALLOC_SZ     1024

/* Driver Private Data - Per Instance --------------------------------*/

typedef struct loopback_data_type {
    int dev_handle;         // globally assigned device handle, 1+
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
    // newly open loopback instances are stashed here. array indexing
    // directs new instances by forming the index into the list.
    // eg. user opens "/dev/loop/1" so instance saved into instlist[1]
    //     generates error if there is already an entry in index 1.
    //     list starts out all pre-filled with NULLS
    // IMPORTANT - data is malloc'd and so must be free'd when closed.
    loopback_data_t * instlist[MAX_LOOPBACK_INSTANCES];

} loopback_ctx_t;

static loopback_ctx_t _loopback_ctx;

static const char driver_name_prefix[] = "/dev/loop/";


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
                    rc = loopinst->dev_handle;
#ifdef TDD_PRINTF
                    printf("{loopdev_open} minor[%d] OPENED.\n", minor);
#endif    
                }
            }
        }
    }
    return rc;
}


// close( (void *) opctx ) --> STATUS
static int loopdev_close(void * opctx) {
    int rc = -1;
#ifdef TDD_PRINTF
    printf("{loopdev_close}\n");
#endif    
    if (opctx) {
        loopback_data_t * loopinst = (loopback_data_t *)opctx;
        // use the internal index to find the registration in the driver
        // and then close out the instance. Loopback needs only to 
        // delete the underlying memory.
        int idx = loopinst->inst_index;
#ifdef TDD_PRINTF
        printf("{loopdev_close} minor[%d]\n", idx);
#endif    
        if (_loopback_ctx.instlist[idx] == loopinst) {
            // memory confirmed. take it down
            free( _loopback_ctx.instlist[idx]->in_buffer );
            free( _loopback_ctx.instlist[idx]->out_buffer );
            free( _loopback_ctx.instlist[idx] );
            _loopback_ctx.instlist[idx] = NULL;
#ifdef TDD_PRINTF
            printf("{loopdev_close} minor[%d] CLOSED.\n", idx);
#endif    
            rc = 0;
        }
    }
    return rc;
}


// read( (void *) opctx, (char *)buffer, (int)len ) --> readcount | STATUS
static int loopdev_read(void * opctx, char * buf, int maxlen) {
    int rc = -1;
#ifdef TDD_PRINTF
    printf("{loopdev_read}\n");
#endif    
    if (opctx) {
        loopback_data_t * inst = (loopback_data_t *)opctx;
        rc = 0;
        while (maxlen && (inst->in_len > 0)  ) {
            *(buf ++) = inst->in_buffer[ inst->in_head ];
            if (++inst->in_head >= inst->in_buflen) {
                inst->in_head = 0;
            }
            maxlen --;
            inst->in_len --;
            rc ++;
        }
    }
    return rc;
}


// write( (void *) opctx, (char *)buffer, (int)len ) --> writecount | STATUS
static int loopdev_write(void * opctx, char * buf, int len) {
    int rc = -1;
#ifdef TDD_PRINTF
    printf("{loopdev_write}\n");
#endif    
    if (opctx) {
        loopback_data_t * inst = (loopback_data_t *)opctx;
        rc = 0;
        while (len && (inst->out_len < inst->out_buflen)) {
            inst->out_buffer[ inst->out_tail ] = *(buf ++);
            if (++inst->out_tail >= inst->out_buflen) {
                inst->out_tail = 0;
            }
            len --;
            inst->out_len ++;
            rc ++;
        }
    }
    return rc;
}


// reset( (void *) opctx, ) --> STATUS
static int loopdev_reset(void * opctx) {
    int rc = -1;
#ifdef TDD_PRINTF
    printf("{loopdev_reset}\n");
#endif    
    if (opctx) {
        loopback_data_t * inst = (loopback_data_t *)opctx;
        inst->in_head = inst->in_tail = inst->in_len = 0;
        inst->out_head = inst->out_tail = inst->out_len = 0;
        rc = 0;
    }
    return rc;
}


// Loopback device has no I/O control commands
// ioctl( (void *) opctx, (int)cmd, (int *)val ) --> STATUS
static int loopdev_ioctl(void * opctx, int cmd, int * val) {
    int rc = -1;
#ifdef TDD_PRINTF
    printf("{loopdev_ioctl}\n");
#endif    
    if (opctx) {
        rc = 0;
    }
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
    0,                          // handle, idx to context table
    (void *)&_loopback_ctx,     // driver-class opaque data     (global)
    NULL                        // driver-instance opaque data  (ctx, loopback_data_t*)
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
// Return: treat as a bool. True := instance established.
int mock_loop_check_instance(int instance) {
#ifdef TDD_PRINTF
    printf("{mock_loop_check_instance} inst[%d]\n", instance);
#endif    
    return (_loopback_ctx.instlist[instance]) ? 1 : 0;
}

int mock_loop_write(int instance, const char * buf, int len) {
    int rc = -1;
#ifdef TDD_PRINTF
    printf("{mock_loop_write} inst[%d] :: ---> [%s]\n", instance, buf);
#endif    
    if (_loopback_ctx.instlist[instance]) {
        loopback_data_t * inst = _loopback_ctx.instlist[instance];
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
#ifdef TDD_PRINTF
    printf("{mock_loop_read} inst[%d]\n", instance);
#endif    
    if (_loopback_ctx.instlist[instance]) {
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
#ifdef TDD_PRINTF
	if (rc > 0) {
		buf[rc] = '\0';
		printf("{mock_loop_read} :: <--- [%s]\n", buf);
	}
#endif    
    return rc;
}


