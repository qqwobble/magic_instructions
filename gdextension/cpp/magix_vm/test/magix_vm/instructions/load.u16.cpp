#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.u16")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.u16 1
@entry:
load.u16 $0, #data
__unittest.put.u16 $0
exit
)",
        magix::u16{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 0xffff", UR"(
data:
.u16 0xffff
@entry:
load.u16 $0, #data
__unittest.put.u16 $0
exit
)",
        magix::u16{0xffff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.u16 1
@entry:
load.u16 $0, #data
__unittest.put.u16 $0
exit
)",
        magix::u16{1}
    );
}
