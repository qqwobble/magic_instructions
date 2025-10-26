#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.i64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.i64 1
@entry:
load.i64 $0, #data
__unittest.put.i64 $0
exit
)",
        magix::i64{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 0x7fffffffffffffff", UR"(
data:
.i64 0x7fffffffffffffff
@entry:
load.i64 $0, #data
__unittest.put.i64 $0
exit
)",
        magix::i64{0x7fffffffffffffff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load -0x8000000000000000", UR"(
data:
.i64 -0x8000000000000000
@entry:
load.i64 $0, #data
__unittest.put.i64 $0
exit
)",
        magix::i64{-0x7fffffffffffffff - 1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.i64 1
@entry:
load.i64 $0, #data
__unittest.put.i64 $0
exit
)",
        magix::i64{1}
    );
}
