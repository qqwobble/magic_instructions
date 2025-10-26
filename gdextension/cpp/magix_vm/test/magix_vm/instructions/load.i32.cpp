#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.i32")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.i32 1
@entry:
load.i32 $0, #data
__unittest.put.i32 $0
exit
)",
        magix::i32{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 0x7fffffff", UR"(
data:
.i32 0x7fffffff
@entry:
load.i32 $0, #data
__unittest.put.i32 $0
exit
)",
        magix::i32{0x7fffffff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load -0x80000000", UR"(
data:
.i32 -0x80000000
@entry:
load.i32 $0, #data
__unittest.put.i32 $0
exit
)",
        magix::i32{-0x7fffffff - 1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.i32 1
@entry:
load.i32 $0, #data
__unittest.put.i32 $0
exit
)",
        magix::i32{1}
    );
}
