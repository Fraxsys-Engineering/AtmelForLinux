/*
 * test_cmdparser.c
 *
 * TDD For avrlib/cmdparser.*
 *
 * Supports lib ver: 1.0
 *
 */


#include <avrlib/cmdparser.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


static char test_feedback_buffer[256];

/* --- command parser function prototype ------------------------------
typedef int (*cmdparsefcn)(int, const char **);
---------------------------------------------------------------------- */

/* --- command structure ----------------------------------------------
typedef struct cmdobj_type {
    static const char * noun;       /* primary key (noun) long form 
    static const char * nsc;        /* optional noun shortened form 
    unsigned int        verb_min;   /* minimum # expected verbs     
    unsigned int        verb_max;   /* maximum # expected verbs     
    cmdparsefcn         cp;         /* function to call to parse    
} cmdobj;
---------------------------------------------------------------------- */

static int cmd_foo(int vc, const char * verbs[]) {
	char vb1[20];
	strcpy(vb1,"<nil>");
	
	if (vc == 1) {
		strcpy(vb1,verbs[0]);
	}
	sprintf( test_feedback_buffer, "(cmd_foo) vc:%d v1:%s\n", vc, vb1);
	return 0;
}

static int cmd_bar(int vc, const char * verbs[]) {
	sprintf( test_feedback_buffer, "(cmd_bar) vc:%d\n", vc);
	return 0;
}

static const cmdobj pCommandList[] = {
	{ "foo", NULL, 0, 1, cmd_foo},
	{ "bar", "b",  0, 0, cmd_bar},
	{ NULL, NULL,  0, 0, NULL}
};

// ======== TEST SUITE ================================================

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void test_cmdparser(void) {

	CU_ASSERT( startParser(NULL, 0) == 0);


}

int main() {

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

