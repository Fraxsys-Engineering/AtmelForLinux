/*
 * test_cmdparser.c
 *
 * TDD For avrlib/cmdparser.*
 *
 * Supports lib ver: 1.0
 *
 */

#include <avrlib/driver.h>
#include <avrlib/cmdparser.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

// MOCK backend hooks, for testing and inserting character stream
// data, etc.
extern int mock_loop_check_instance(int instance);
extern int mock_loop_write(int instance, const char * buf, int len);
extern int mock_loop_read(int instance, char * buf, int maxlen);
extern int mock_loop_reset(int instance);

//static char test_feedback_buffer[256];

/* --- command parser function prototype ------------------------------
typedef int (*cmdparsefcn)(int, const char **);
---------------------------------------------------------------------- */

/* --- command structure ----------------------------------------------
typedef struct cmdobj_type {
    const char * noun;              primary key (noun) long form 
    const char * nsc;               optional noun shortened form 
    unsigned int        verb_min;   minimum # expected verbs     
    unsigned int        verb_max;   maximum # expected verbs     
    cmdparsefcn         cp;         function to call to parse    
} cmdobj;
---------------------------------------------------------------------- */

static int cmd_foo(int vc, const char * verbs[]) {
	char vb1[20];
	strcpy(vb1,"<nil>");
	
	pSendString("cmd_foo\r\n");
    if (vc == 1) {
		strcpy(vb1,verbs[0]);
        pSendString("/");
        pSendString(vb1);
	}
	printf("(cmd_foo) vc:%d v1:%s\n", vc, vb1);
	return CMD_SUCCESS;
}

static int cmd_bar(int vc, const char * verbs[]) {
	printf("(cmd_bar) vc:%d\n", vc);
    pSendString("cmd_bar\r\n");
	return CMD_SUCCESS;
}

cmdobj pCommandList[3] = {
	{ "foo", NULL, 0, 1, cmd_foo},
	{ "bar", "b",  0, 0, cmd_bar},
	{ NULL, NULL,  0, 0, NULL}
};

static int StringContains(const char * strn, const char * has) {
    return (strn && has && strstr(strn,has) != NULL) ? 1 : 0;
}

// ======== TEST SUITE ================================================

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void test_cmdparser(void) {
    int  len;
	int  inst = 0;
	char rxb[128];

    char pHeader[] = "AVRLIB Command Parser Version 1.0\r\n";
    char cmdbad[] = "bart\r\n";
    char cmd1[] = "foo\r\n";
    char cmd1_resp[] = "cmd_foo";
    char resp_ok[] = "\r\nOK\r\n";
    char resp_err[] = "Error (";
    

	printf("\n"); // CUnit annoyingly does not CR when adding a print stating that it is starting this test...
	
	sprintf(rxb,"/dev/loop/%d",inst);
	printf("{test_cmdparser} Opening device: %s on parser...\n", rxb);
	CU_ASSERT( startParser(rxb, 0) == 0);
	
	// make sure loopback instance <inst> was created.
	CU_ASSERT_FATAL( mock_loop_check_instance(inst) == 1 );

    // loopback should be holding the Parser "login" header 'pHeader'
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Parser Header :: %s", rxb);
    CU_ASSERT_STRING_EQUAL_FATAL(rxb,pHeader);
	
    // more operations, driven by the polling loop.
    mock_loop_write(inst,cmdbad,strlen(cmdbad));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,resp_err) );

    mock_loop_write(inst,cmd1,strlen(cmd1));
    pollParser();
    len = mock_loop_read(inst,rxb,128);
    rxb[len] = '\0';
    printf("{test_cmdparser} (readback) Cmd Resp :: %s", rxb);
    CU_ASSERT_FATAL( StringContains(rxb,cmd1_resp) );
    CU_ASSERT_FATAL( StringContains(rxb,resp_ok) );
    
    
    // just exit, parser cannot be closed...

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
        
    pSuite = CU_add_suite("Test Suite - stringutils", init_suite, clean_suite);
    if (pSuite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ( !CU_add_test(pSuite, "test cmdparser / foo & bar", test_cmdparser) ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

