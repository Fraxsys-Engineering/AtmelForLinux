/*
 * test_chardriverstack.c
 *
 * TDD For avrlib/(character driver stack with loopback)
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


// This function is in the loopback driver source code and must be 
// called to hookup the driver into the registered driver list.
extern int loopback_Register(void);

// MOCK backend hooks, for testing and inserting character stream
// data, etc.
extern int mock_loop_check_instance(int instance);
int mock_loop_write(int instance, const char * buf, int len);
int mock_loop_read(int instance, char * buf, int maxlen);

// ======== TEST SUITE ================================================

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void test_chardriverstack(void) {
	int inst = 0;
	const char ts1[] = "test string1\n";
	const char ts2[] = "test string2\n";
	char rxb[32];
	int hnd  = -1;
	
	printf("\n"); // CUnit annoyingly does not CR when adding a print stating that it is starting this test...
	
	sprintf(rxb,"/dev/loop/%d",inst);
	printf(" Opening device: %s\n", rxb);
	hnd = cd_open(rxb, 0);
	CU_ASSERT_FATAL( hnd >= 0 );
	
	// make sure loopback instance <inst> was created.
	CU_ASSERT_FATAL( mock_loop_check_instance(inst) == 1 );
	
	// put a receiving string into the loopback driver's Rx buffer in loop/0
	// then read it using the serial stack's API
	printf("Testing full RECEIVE - placing data into loopback RX buffer :: %s\n", ts1);
	CU_ASSERT_FATAL( mock_loop_write(inst,ts1,strlen(ts1)+1) == strlen(ts1)+1 );
	CU_ASSERT_FATAL( cd_read(hnd,rxb,32) == strlen(ts1)+1 );
	printf("  readback :: %s\n", rxb);
	CU_ASSERT_STRING_EQUAL_FATAL(rxb,ts1);

	// write a string out on the serial stack's API then pull the contents
	// of the loopback driver's Tx buffer and compare it.
	printf("Testing full TRANSMIT - sending :: %s\n", ts2);
	CU_ASSERT_FATAL( cd_write(hnd,ts2,strlen(ts2)+1) == strlen(ts2)+1 );
	CU_ASSERT_FATAL( mock_loop_read(inst,rxb,32) == strlen(ts2)+1 );
	printf("  readback :: %s\n", rxb);
	CU_ASSERT_STRING_EQUAL_FATAL(rxb,ts2);

	cd_close(hnd);
}

int main() {
	// char strean driver stack - system init
	printf("{TDD} Driver Stack Init...\n");
	System_DriverStartup();

	// Register loopback driver to get it into the driver stack...
	printf("{TDD} Registering the loopback driver...\n");
	loopback_Register();

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

