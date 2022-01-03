/*
 * test_parser.c
 *
 * TDD For avrlib - command parser attached to emulated serial driver
 *
 * Supports lib ver: 1.0
 *
 */

#include <avrlib/driver.h>      // needed in order to setup the drivers
#include <avrlib/cmdparser.h>   // this wraps the character device (chardev.h) header include not needed in this test code.
#include <avrlib/libtime.h>     // time functions, emulation friendly.
#include <stdio.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#ifndef UART_ENABLE_EMU_1
 #error "EMULATED UART MINOR DEVICE 1 HAS TO BE ENABLED USING DEFINE: UART_ENABLE_EMU_1 !"
#endif

// MOCK backend hooks, for testing and inserting character stream
// data, etc.
extern uint8_t uart_emu1_rd_dummyreg_udr1(void);
extern uint8_t uart_emu1_rd_dummyreg_ucsr1a(void);
extern uint8_t uart_emu1_rd_dummyreg_ucsr1b(void);
extern uint8_t uart_emu1_rd_dummyreg_ucsr1c(void);
extern uint8_t uart_emu1_rd_dummyreg_ubrr1h(void);
extern uint8_t uart_emu1_rd_dummyreg_ubrr1l(void);
extern int uart_emu1_get_tx(char * strn, int maxread);
extern int uart_emu1_put_rx(const char * strn, int len);

// Testing Command Parser

// command parser "foo" No verbs.
static int cmdpfunc_foo(int verbc, const char * verbv[]) {
    pSendString("recvd: foo\r\n");
    return CMD_SUCCESS;
}

static cmdobj cmdobj_foo = {
    "foo",              /* long form name  */
    NULL,               /* short form name */
    0,                  /* min. verbs      */
    0,                  /* max. verbs      */
    cmdpfunc_foo        /* parser fcn      */
};

// command parser "bar" 1-2 verbs
static int cmdpfunc_bar(int verbc, const char * verbv[]) {
    int i;
    pSendString("recvd: bar - verbcount[");
    pSendInt(verbc);
    pSendString("]");
    if (verbc > 0) {
        pSendString("] {");
        for ( i = 0 ; i < verbc ; ++i ) {
            pSendString(verbv[i]);
            if (i < verbc-1)
                pSendString(" ");
        }
        pSendString("}");
    }
    pSendString("\r\n");
    return CMD_SUCCESS;
}

static cmdobj cmdobj_bar = {
    "bar",              /* long form name  */
    "b",                /* short form name */
    1,                  /* min. verbs      */
    2,                  /* max. verbs      */
    cmdpfunc_bar        /* parser fcn      */
};


cmdobj * const pCommandList[] = { &cmdobj_foo, &cmdobj_bar, NULL };


// ======== TEST SUITE ================================================

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void test_chardriverstack(void) {
	int minor = 1;  // uart/1
    char rxb[32];
    
    printf("\n"); // CUnit annoyingly does not CR when adding a print stating that it is starting this test...
    
    sprintf(rxb,"/dev/uart/%d,9600,8,N,1",minor);
    printf(" Opening device: %s (via Parser)\n", rxb);
    startParser(rxb, 0);
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int inst = 1;
	const char ts1[] = "test string1\n";
	const char ts2[] = "test string2\n";
	
	int hnd  = -1;
	
	
	
	printf(" Opening device: %s\n", rxb);
	hnd = cd_open(rxb, 0);
	CU_ASSERT_FATAL( hnd >= 0 );
	
    
	// put a receiving string into the loopback driver's Rx buffer in loop/0
	// then read it using the serial stack's API
	printf("Testing full RECEIVE - placing data into emulated RX buffer :: %s\n", ts1);
	CU_ASSERT_FATAL( uart_emu1_put_rx(ts1,strlen(ts1)+1) == strlen(ts1)+1 );
	CU_ASSERT_FATAL( cd_read(hnd,rxb,32) == strlen(ts1)+1 );
	printf("  readback :: %s\n", rxb);
	CU_ASSERT_STRING_EQUAL_FATAL(rxb,ts1);

	// write a string out on the serial stack's API then pull the contents
	// of the loopback driver's Tx buffer and compare it.
	printf("Testing full TRANSMIT - sending :: %s\n", ts2);
	CU_ASSERT_FATAL( cd_write(hnd,ts2,strlen(ts2)+1) == strlen(ts2)+1 );
	CU_ASSERT_FATAL( uart_emu1_get_tx(rxb,32) == strlen(ts2)+1 );
	printf("  readback :: %s\n", rxb);
	CU_ASSERT_STRING_EQUAL_FATAL(rxb,ts2);

	cd_close(hnd);
}

int main() {
	// char strean driver stack - system init
	printf("{TDD} Driver Stack Init...\n");
	printf("{TDD} Registering the loopback driver...\n");
	System_DriverStartup();

	printf("{TDD} Initializing the Drivers...\n");
	System_driverInit();

    

    CU_pSuite pSuite = NULL;
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
        
    pSuite = CU_add_suite("Test Suite - character driver stack (loopback)", init_suite, clean_suite);
    if (pSuite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ( !CU_add_test(pSuite, "test driver-stack / loopback", test_chardriverstack) ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

