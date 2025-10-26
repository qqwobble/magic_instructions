#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.i16")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.i16 1
@entry:
load.i16 $0, #data
__unittest.put.i16 $0
exit
)",
        magix::i16{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 0x7fff", UR"(
data:
.i16 0x7fff
@entry:
load.i16 $0, #data
__unittest.put.i16 $0
exit
)",
        magix::i16{0x7fff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load -0x8000", UR"(
data:
.i16 -0x8000
@entry:
load.i16 $0, #data
__unittest.put.i16 $0
exit
)",
        magix::i16{-0x8000}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.i16 1
@entry:
load.i16 $0, #data
__unittest.put.i16 $0
exit
)",
        magix::i16{1}
    );
}
