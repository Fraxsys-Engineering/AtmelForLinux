/*
 * test_emuuart.c
 *
 * TDD For avrlib/(character driver stack with emulated AVR UART Driver)
 *
 * Supports lib ver: 1.0
 *
 */

#include <avrlib/chardev.h>
#include <avrlib/driver.h>
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


// ======== TEST SUITE ================================================

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void test_chardriverstack(void) {
	int inst = 1;
	const char ts1[] = "test string1\n";
	const char ts2[] = "test string2\n";
	char rxb[32];
	int hnd  = -1;
	
	printf("\n"); // CUnit annoyingly does not CR when adding a print stating that it is starting this test...
	
    // All these device open attempts should FAIL
    CU_ASSERT_FATAL( cd_open("/dev/part/1",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/5",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/1,100",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/1,9600,",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/1,9600,10",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/1,9600,8,",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/1,9600,8,Q",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/1,9600,8,N,",0) < 1 );
    CU_ASSERT_FATAL( cd_open("/dev/uart/1,9600,5,E,3",0) < 1 );
    
	sprintf(rxb,"/dev/uart/%d,9600,8,N,1",inst);
	printf(" Opening device: %s\n", rxb);
	hnd = cd_open(rxb, 0);
	CU_ASSERT_FATAL( hnd >= 0 );
	
    printf("Examining UART Register settings made...\n");
    printf("  UDR1[0x%02x]  UCSR1A[0x%02x]  UCSR1B[0x%02x]  UCSR1C[0x%02x]  UBRR1H[0x%02x]  UBRR1L[0x%02x]\n",
        uart_emu1_rd_dummyreg_udr1(), uart_emu1_rd_dummyreg_ucsr1a(), 
        uart_emu1_rd_dummyreg_ucsr1b(), uart_emu1_rd_dummyreg_ucsr1c(),
        uart_emu1_rd_dummyreg_ubrr1h(), uart_emu1_rd_dummyreg_ubrr1l() );
    
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

