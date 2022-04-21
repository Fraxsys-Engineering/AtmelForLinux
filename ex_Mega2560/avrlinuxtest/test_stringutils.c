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

void test_simple_string_ops(void) {
    char inbuf[32];
    char outbuf[32];
    
    // string lengths
    CU_ASSERT( sutil_strlen("") == 0 );
    CU_ASSERT( sutil_strlen("a") == 1 );
    CU_ASSERT( sutil_strlen("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef") == 256 );

    strcpy(inbuf,"test copy string");
    CU_ASSERT( sutil_strcpy(outbuf,inbuf) == strlen(inbuf) );
    CU_ASSERT_STRING_EQUAL(inbuf,outbuf);

}

void test_string_to_num(void) {
    char haystack1[] = "qwertyuio";
    char haystack2[] = "asdfghjkl";
    char hexstrn1[]  = "21df";
    char hexstrn2[]  = "0x21DF";
    
    CU_ASSERT( sutil_strchar(haystack1,'u') == 6  );
    CU_ASSERT( sutil_strchar(haystack2,'u') == -1 );
    CU_ASSERT( sutil_ishexstring(hexstrn1) == 1   );
    CU_ASSERT( sutil_ishexstring(hexstrn2) == 1   );
    CU_ASSERT( sutil_ishexstring(haystack2) == 0  );
    CU_ASSERT( sutil_strtohex(hexstrn1) == 0x21df );
    CU_ASSERT( sutil_strtohex(hexstrn2) == 0x21df );
    CU_ASSERT( sutil_strtohex(hexstrn1+2) == 0xdf );

}

//char * sutil_strtok(char * b, const char * m);
void test_sutil_strtok(void) {
    char testbuffer[40];
    char matchbuf[] = ", \r\n";
    char * b = &(testbuffer[0]);
    char * n;
    strcpy(testbuffer, "test  string sentence,one, two\r\n");
    
    //printf("\n");
    //printf("testbuffer[%s]\n", testbuffer);
    //printf("b[%s]\n", b);
    //printf("matchbuf[%s] len[%d]\n", matchbuf, strlen(matchbuf));
    
    n = sutil_strtok(b,matchbuf);
    //printf("n[%s] b[%s]\n", (n)?n:"<NULL>", (b)?b:"<NULL>");
    CU_ASSERT_FATAL( n && *n == 's' );
    CU_ASSERT_STRING_EQUAL_FATAL(b,"test");
    b = n;
    
    n = sutil_strtok(b,matchbuf);
    //printf("n[%s] b[%s]\n", (n)?n:"<NULL>", (b)?b:"<NULL>");
    CU_ASSERT_FATAL( n && *n == 's' );
    CU_ASSERT_STRING_EQUAL_FATAL(b,"string");
    b = n;
    
    n = sutil_strtok(b,matchbuf);
    //printf("n[%s] b[%s]\n", (n)?n:"<NULL>", (b)?b:"<NULL>");
    CU_ASSERT_FATAL( n && *n == 'o' );
    CU_ASSERT_STRING_EQUAL_FATAL(b,"sentence");
    b = n;
    
    n = sutil_strtok(b,matchbuf);
    //printf("n[%s] b[%s]\n", (n)?n:"<NULL>", (b)?b:"<NULL>");
    CU_ASSERT_FATAL( n && *n == 't' );
    CU_ASSERT_STRING_EQUAL_FATAL(b,"one");
    b = n;
    
    n = sutil_strtok(b,matchbuf);
    //printf("n[%s] b[%s]\n", (n)?n:"<NULL>", (b)?b:"<NULL>");
    CU_ASSERT_FATAL( n && *n == '\0' );
    CU_ASSERT_STRING_EQUAL_FATAL(b,"two");
    b = n;
    
    n = sutil_strtok(b,matchbuf);
    //printf("n[%s] b[%s]\n", (n)?n:"<NULL>", (b)?b:"<NULL>");
    CU_ASSERT_FATAL( n == NULL );
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

void test_stringutils_sutil_asciinumber(void) {
    char strbuf[12];
    uint8_t ret;

    //printf("\ntest_stringutils_sutil_asciinumber -------------- \n");
    // Zero
	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,0);
	CU_ASSERT( ret == 1);
	CU_ASSERT_STRING_EQUAL(strbuf,"0");
    //printf("VALUE[0] --> len[%d] :: %s\n", (int)ret, strbuf);

    // Small numbers (+/-)
	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,1);
	CU_ASSERT(ret == 1);
	CU_ASSERT_STRING_EQUAL(strbuf,"1");
    //printf("VALUE[1] --> len[%d] :: %s\n", (int)ret, strbuf);

	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,-1);
	CU_ASSERT(ret == 2);
	CU_ASSERT_STRING_EQUAL(strbuf,"-1");
    //printf("VALUE[-1] --> len[%d] :: %s\n", (int)ret, strbuf);

    // Zero padding (+/-)
	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,12003);
	CU_ASSERT(ret == 5);
	CU_ASSERT_STRING_EQUAL(strbuf,"12003");
    //printf("VALUE[12003] --> len[%d] :: %s\n", (int)ret, strbuf);

	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,-3200004);
	CU_ASSERT(ret == 8);
	CU_ASSERT_STRING_EQUAL(strbuf,"-3200004");
    //printf("VALUE[-3200004] --> len[%d] :: %s\n", (int)ret, strbuf);

    // Range maximums (+/-)
	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,2147483647);
	CU_ASSERT(ret == 10);
	CU_ASSERT_STRING_EQUAL(strbuf,"2147483647");
    //printf("VALUE[2147483647] --> len[%d] :: %s\n", (int)ret, strbuf);

	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,-2147483647);
	CU_ASSERT(ret == 11);
	CU_ASSERT_STRING_EQUAL(strbuf,"-2147483647");
    //printf("VALUE[-2147483647] --> len[%d] :: %s\n", (int)ret, strbuf);

	memset(strbuf,0,sizeof(strbuf));
    ret = sutil_asciinumber(strbuf,-2147483648);
	CU_ASSERT(ret == 11);
	CU_ASSERT_STRING_EQUAL(strbuf,"-2147483648");
    //printf("VALUE[-2147483648] --> len[%d] :: %s\n", (int)ret, strbuf);
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

    if ( !CU_add_test(pSuite, "test stringutils / test_simple_string_ops()", test_simple_string_ops)            ||
         !CU_add_test(pSuite, "test stringutils / string to number",      test_string_to_num)                   ||
         !CU_add_test(pSuite, "test stringutils / sutil_nibble()",        test_stringutils_sutil_nibble)        ||
         !CU_add_test(pSuite, "test stringutils / sutil_strtok()",        test_sutil_strtok)                    ||
         !CU_add_test(pSuite, "test stringutils / sutil_asciihex_byte()", test_stringutils_sutil_asciihex_byte) || 
         !CU_add_test(pSuite, "test stringutils / sutil_asciihex_word()", test_stringutils_sutil_asciihex_word) ||
         !CU_add_test(pSuite, "test stringutils / sutil_asciihex_long()", test_stringutils_sutil_asciihex_long) ||
         !CU_add_test(pSuite, "test stringutils / sutil_asciinumber()",   test_stringutils_sutil_asciinumber)   ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

