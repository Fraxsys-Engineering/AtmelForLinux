/*
 * test_gpioapi.c
 *
 * TDD For avrlib/(GPIO API)
 *
 * Supports lib ver: 1.0
 *
 */

#include <avrlib/gpio_api.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


// MOCK backend hooks, for GPIO register access
extern uint8_t MCUCR;
extern uint8_t TEST_REGISTERS[6];
extern sfr8p_t TR_ADDR_BASE;


// ======== TEST SUITE ================================================

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

static void print_bin_byte( uint8_t byt ) {
    int i;
    for (i = 0 ; i < 8 ; ++i ) {
        if (byt & 0x80) {
            printf("1");
        } else {
            printf("0");
        }
        byt <<= 1;
    }
}

static char * portname[3] = {
    "PORTA", "PORTB", ""
};

static void dump_port( uint8_t port ) {
    if (port < TEST_PORT_COUNT) {
        printf("=== [DUMP] ---- %s -----------------------------------\n", portname[port]);
        
        printf("      [HH] (76543210)\n");
        
        printf("  DDR [%02x] (", TEST_REGISTERS[(port*3)+1]);
        print_bin_byte(TEST_REGISTERS[(port*3)+1]);
        printf(")\n");
        
        printf(" PORT [%02x] (", TEST_REGISTERS[(port*3)+2]);
        print_bin_byte(TEST_REGISTERS[(port*3)+2]);
        printf(")\n");
        
        printf("\n");
    }
}

static int thndl = 1;

void test_gpio_bits(void) {
    int var;
    int pa0 = pm_register_pin(PM_PORT_A, PM_PIN_0, PINMODE_INPUT_TRI);
    int pa1 = pm_register_pin(PM_PORT_A, PM_PIN_1, PINMODE_INPUT_PU);
    int pa4 = pm_register_pin(PM_PORT_A, PM_PIN_4, PINMODE_OUTPUT_LO);
    int pa6 = pm_register_pin(PM_PORT_A, PM_PIN_6, PINMODE_OUTPUT_HI);
    int pa_general = 0;
    int pa7 = 0;
    
    printf("\n");    
    printf("[test_gpio_bits] ----------------------------------------\n");
    CU_ASSERT_FATAL ( pa0 == thndl++ );
    CU_ASSERT_FATAL ( pa1 == thndl++ );
    CU_ASSERT_FATAL ( pa4 == thndl++ );
    CU_ASSERT_FATAL ( pa6 == thndl++ );
    /* DDR: pins 4,6 are outputs '1' */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0x50 );
    /* PORT: pin6 = out-hi (1), pin1 = in-pullup ('1' written to port pin when set to an input) */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x42 );
    printf("Initial setup, PA0 = input-tri, PA1 = input-pu, PA4 = out-lo, PA6 = out-hi\n");
    dump_port(PM_PORT_A);
    
    printf("Try to re-use PA0...\n");
    pa_general = pm_register_pin(PM_PORT_A, PM_PIN_0, PINMODE_UNCHANGED);
    CU_ASSERT_FATAL ( pa_general == PM_ERROR );
    pa_general = 0;
    
    printf("Try to use Port0 as a full 8-bit port...\n");
    pa_general = pm_register_prt(PM_PORT_A, 0xAA, PINMODE_OUTPUT_LO);
    CU_ASSERT_FATAL ( pa_general == PM_ERROR );
    pa_general = 0;
    
    printf("Add another GPIO pin - PA7 (out_lo)\n");
    pa7 = pm_register_pin(PM_PORT_A, PM_PIN_7, PINMODE_OUTPUT_LO);
    CU_ASSERT_FATAL ( pa7 == thndl++ );
    dump_port(PM_PORT_A);
    /* DDR: pins 4,6,7 are outputs '1' */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0xd0 );
    /* PORT: pin6 = out-hi (1), pin1 = in-pullup ('1' written to port pin when set to an input) */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x42 );

    printf("Set PA7 (out) to hi\n");
    CU_ASSERT_FATAL ( pm_out(pa7, 1) == PM_SUCCESS );
    dump_port(PM_PORT_A);
    /* DDR: pins 4,6,7 are outputs '1' */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0xd0 );
    /* PORT: pin6,7 = out-hi (1), pin1 = in-pullup ('1' written to port pin when set to an input) */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0xc2 );

    printf("Set PA6 (out) to lo\n");
    CU_ASSERT_FATAL ( pm_out(pa6, 0) == PM_SUCCESS );
    dump_port(PM_PORT_A);
    /* DDR: pins 4,6,7 are outputs '1' */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0xd0 );
    /* PORT: pin6,7 = out-hi (1), pin1 = in-pullup ('1' written to port pin when set to an input) */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x82 );

    printf("Set PA6 (out) to lo (duplicate, should not change)\n");
    CU_ASSERT_FATAL ( pm_out(pa6, 0) == PM_SUCCESS );
    dump_port(PM_PORT_A);
    /* DDR: pins 4,6,7 are outputs '1' */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0xd0 );
    /* PORT: pin6,7 = out-hi (1), pin1 = in-pullup ('1' written to port pin when set to an input) */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x82 );

    printf("Switch PA7 to a Hi-Z input (no pullup)\n");
    CU_ASSERT_FATAL ( pm_chg_dir(pa7, PINMODE_INPUT_TRI) == PM_SUCCESS );
    dump_port(PM_PORT_A);
    /* DDR: pins 4,6 are outputs '1' */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0x50 );
    /* PORT: pin6,7 = out-hi (1), pin1 = in-pullup ('1' written to port pin when set to an input) */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x02 );

    printf("Switch PA6 to a pullup input (needs '1' in port bit)\n");
    CU_ASSERT_FATAL ( pm_chg_dir(pa6, PINMODE_INPUT_PU) == PM_SUCCESS );
    dump_port(PM_PORT_A);
    /* DDR: pins 4,6 are outputs '1' */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0x10 );
    /* PORT: pin6,7 = out-hi (1), pin1 = in-pullup ('1' written to port pin when set to an input) */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x42 );

    printf("Read PA0 (current state is low)\n");
    CU_ASSERT_FATAL ( pm_in(pa0) == 0 );
    dump_port(PM_PORT_A);
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0x10 );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x42 );

    var = TEST_REGISTERS[(PM_PORT_A*3)+2] | 1;
    TEST_REGISTERS[(PM_PORT_A*3)+2] = var;
    dump_port(PM_PORT_A);
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0x10 );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x43 );
    
    printf("Read PA0 (current state is high)\n");
    CU_ASSERT_FATAL ( pm_in(pa0) == 1 );
    dump_port(PM_PORT_A);
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0x10 );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x43 );

    printf("Toggle bit 4 (simulated, check PINA4 for change)\n");
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+0] == 0 );
    CU_ASSERT_FATAL ( pm_tog(pa4) == PM_SUCCESS );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+0] == 0x10 );
    dump_port(PM_PORT_A);
    TEST_REGISTERS[(PM_PORT_A*3)+0] = 0;
    
    printf("[test_gpio_bits] - COMPLETED\n");
    //s_setmode_pin(s_portstatus * pp, uint8_t pin, uint8_t mode)
}

void test_gpio_port(void) {
    int var;
    int portb = pm_register_prt(PM_PORT_B, 0xAA, PINMODE_OUTPUT_LO);
    int port_general = 0;

    printf("\n");
    printf("[test_gpio_port] PortB -----------------------------------\n");
    CU_ASSERT_FATAL ( portb == thndl++ );
    dump_port(PM_PORT_B);
    /* port a should not have changed */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+1] == 0x10 );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_A*3)+2] == 0x43 );
    /* initial portb assignments */
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_B*3)+1] == 0xff );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_B*3)+2] == 0xaa );

    printf("Try to register a bit on PortB \n");
    port_general = pm_register_pin(PM_PORT_B, PM_PIN_0, PINMODE_UNCHANGED);
    CU_ASSERT_FATAL ( port_general == PM_ERROR );
    port_general = 0;

    printf("Try to register another PortB \n");
    port_general = pm_register_prt(PM_PORT_B, 0x55, PINMODE_OUTPUT_LO);
    CU_ASSERT_FATAL ( port_general == PM_ERROR );
    port_general = 0;

    printf("Read PortB \n");
    var = pm_in(portb);
    dump_port(PM_PORT_B);
    CU_ASSERT_FATAL ( var == 0xaa );
    
    printf("Write PortB \n");
    CU_ASSERT_FATAL ( pm_out(portb,0x15) == PM_SUCCESS );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_B*3)+1] == 0xff );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_B*3)+2] == 0x15 );
    dump_port(PM_PORT_B);

    printf("Toggle PortB (simulated, check PINB for all 1's)\n");
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_B*3)+0] == 0 );
    CU_ASSERT_FATAL ( pm_tog(portb) == PM_SUCCESS );
    CU_ASSERT_FATAL ( TEST_REGISTERS[(PM_PORT_B*3)+0] == 0xff );
    dump_port(PM_PORT_B);
    TEST_REGISTERS[(PM_PORT_B*3)+0] = 0;
    
    printf("Change PortB to all inputs\n");
    CU_ASSERT_FATAL ( pm_chg_dir(portb,PINMODE_INPUT_TRI) == PM_SUCCESS );
    dump_port(PM_PORT_B);
}

int main() {
	// system init
	printf("{TDD} System Init...\n");
	pm_init();

    CU_pSuite pSuite = NULL;
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
        
    pSuite = CU_add_suite("Test Suite - GPIO API (PortA,B 8-bits each)", init_suite, clean_suite);
    if (pSuite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ( !CU_add_test(pSuite, "[GPIO_API] Pin (bit) testing", test_gpio_bits) ) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if ( !CU_add_test(pSuite, "[GPIO_API] Port (byte) testing", test_gpio_port) ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

