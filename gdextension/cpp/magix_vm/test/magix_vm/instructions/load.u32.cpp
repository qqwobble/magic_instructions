#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.u32")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.u32 1
@entry:
load.u32 $0, #data
__unittest.put.u32 $0
exit
)",
        magix::u32{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 0xffffffff", UR"(
data:
.u32 0xffffffff
@entry:
load.u32 $0, #data
__unittest.put.u32 $0
exit
)",
        magix::u32{0xffffffff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.u32 1
@entry:
load.u32 $0, #data
__unittest.put.u32 $0
exit
)",
        magix::u32{1}
    );
}
