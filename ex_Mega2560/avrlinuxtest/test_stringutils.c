/*
 * test_stringutils.c
 *
 * TDD For avrlib/stringutils.*
 *
 * Supports lib ver: 1.0
 *
 */


#include <avrlib/stringutils.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void test_stringutils_sutil_nibble(void) {
	uint8_t tval = 0x1F;

	// char sutil_nibble(uint8_t n, uint8_t isHiNibble, uint8_t uc);

	CU_ASSERT_EQUAL( sutil_nibble(tval,SU_HI_NIBBLE,SU_LOWERCASE), '1' );

	CU_ASSERT_EQUAL( sutil_nibble(tval,SU_LO_NIBBLE,SU_LOWERCASE), 'f' );

	CU_ASSERT_EQUAL( sutil_nibble(tval,SU_LO_NIBBLE,SU_UPPERCASE), 'F' );
}

void test_stringutils_sutil_asciihex_byte(void) {
	char strbuf[32];
	uint8_t hval = 0x1F;

	// uint8_t sutil_asciihex_byte(char * at, uint8_t hval, uint8_t prefix, uint8_t uc);
	
	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_byte(strbuf,hval,SU_NO_PREFIX,SU_LOWERCASE) == 2);
	CU_ASSERT_STRING_EQUAL(strbuf,"1f");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_byte(strbuf,hval,SU_USE_PREFIX,SU_LOWERCASE) == 4);
	CU_ASSERT_STRING_EQUAL(strbuf,"0x1f");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_byte(strbuf,hval,SU_NO_PREFIX,SU_UPPERCASE) == 2);
	CU_ASSERT_STRING_EQUAL(strbuf,"1F");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_byte(strbuf,hval,SU_USE_PREFIX,SU_UPPERCASE) == 4);
	CU_ASSERT_STRING_EQUAL(strbuf,"0X1F");
}

void test_stringutils_sutil_asciihex_word(void) {
	char strbuf[32];
	uint16_t hval = 0xDE1F;

	// uint8_t sutil_asciihex_word(char * at, uint16_t hval, uint8_t prefix, uint8_t uc);

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_word(strbuf,hval,SU_NO_PREFIX,SU_LOWERCASE) == 4);
	CU_ASSERT_STRING_EQUAL(strbuf,"de1f");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_word(strbuf,hval,SU_USE_PREFIX,SU_LOWERCASE) == 6);
	CU_ASSERT_STRING_EQUAL(strbuf,"0xde1f");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_word(strbuf,hval,SU_NO_PREFIX,SU_UPPERCASE) == 4);
	CU_ASSERT_STRING_EQUAL(strbuf,"DE1F");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_word(strbuf,hval,SU_USE_PREFIX,SU_UPPERCASE) == 6);
	CU_ASSERT_STRING_EQUAL(strbuf,"0XDE1F");
}

void test_stringutils_sutil_asciihex_long(void) {
	char strbuf[32];
	uint32_t hval = 0xDEADBEEF;

	// uint8_t sutil_asciihex_long(char * at, uint32_t hval, uint8_t prefix, uint8_t uc);

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_long(strbuf,hval,SU_NO_PREFIX,SU_LOWERCASE) == 8);
	CU_ASSERT_STRING_EQUAL(strbuf,"deadbeef");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_long(strbuf,hval,SU_USE_PREFIX,SU_LOWERCASE) == 10);
	CU_ASSERT_STRING_EQUAL(strbuf,"0xdeadbeef");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_long(strbuf,hval,SU_NO_PREFIX,SU_UPPERCASE) == 8);
	CU_ASSERT_STRING_EQUAL(strbuf,"DEADBEEF");

	memset(strbuf,0,sizeof(strbuf));
	CU_ASSERT( sutil_asciihex_long(strbuf,hval,SU_USE_PREFIX,SU_UPPERCASE) == 10);
	CU_ASSERT_STRING_EQUAL(strbuf,"0XDEADBEEF");
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

    if ( !CU_add_test(pSuite, "test stringutils / sutil_nibble()",        test_stringutils_sutil_nibble)        ||
         !CU_add_test(pSuite, "test stringutils / sutil_asciihex_byte()", test_stringutils_sutil_asciihex_byte) || 
         !CU_add_test(pSuite, "test stringutils / sutil_asciihex_word()", test_stringutils_sutil_asciihex_word) ||
         !CU_add_test(pSuite, "test stringutils / sutil_asciihex_long()", test_stringutils_sutil_asciihex_long) ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

