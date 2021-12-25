README - C Unit Testing

Unit testing uses the C Unit Testing Framework "CUnit". For any Debian
based distro, including Ubuntu and Raspbian (Raspberry Pi), the frame-
work can be installed by this command:

    sudo apt-get install libcunit1 libcunit1-doc libcunit1-dev

CUnit testing framework in included by adding this to the TDD "main"
source file:

    #include <CUnit/CUnit.h>
    
Compiling with CUnit test framework requires the following libraries
to be added at compile time:

    -lcunit
   eg. gcc  -o test test.c  -lcunit



BUILDING:

[1] Building one unit test:
	make test_<testname>
	
[2] Building all unit tests:
	make
	
RUNNING UNIT TESTS

[1] Running one test:
	make run_test_<testname>
	
[2] Running all tests:
	make run_all
	

ADDING NEW UNIT TEST SUITES

Each source file in the avr library has its own test suite. In addition
to this, more complicated system tests make use of building multiple
source files then testing an API in the system.

For adding a source unit test, here are the prerequisites and steps:

[1] Required: (new file) unit test "main" file.

    Pattern: test_<srcfile>.c
    eg.      test_stringutils.c
    
    Refer to the stringutils.c test file to see how this file should be
    created. Also refer to CUnit documentation.
    
[2] Makefile: (edit) add a new test target.

	SECTION -A-
	new variable...
	Pattern: TEST_<srcfile> := test_<srcfile>
	
	SECTION -B-
	edit ALL_TESTS - add (append) the new test name here
	
	SECTION-C-
	New test suite source,object lists...
	Pettern: SRC_<srcfile> := (all source files needed for application)
	Pettern: HDR_<srcfile> := (all local and avrlib header files referenced in source)(optional)
	Pettern: OBJ_<srcfile> := (copy and modify OBJ_stringutils)
	(header pattern is optional but allows changes in headers to trigger a rebuild)
	
	SECTION -D-
	edit HEADERS - add (append) HDR_<srcfile> (optional)
	(only do this if you generated a new HDR_* variable in sect. C)




