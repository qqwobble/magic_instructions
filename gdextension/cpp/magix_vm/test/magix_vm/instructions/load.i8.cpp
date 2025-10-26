#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.i8")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.i8 1
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 0x7f", UR"(
data:
.i8 0x7f
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{0x7f}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load -0x80", UR"(
data:
.i8 -0x80
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{-0x80}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.i8 1
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{1}
    );
}
