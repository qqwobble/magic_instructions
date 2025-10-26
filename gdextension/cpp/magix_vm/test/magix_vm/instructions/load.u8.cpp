#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.u8")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.u8 1
@entry:
load.u8 $0, #data
__unittest.put.u8 $0
exit
)",
        magix::u8{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 255", UR"(
data:
.u8 255
@entry:
load.u8 $0, #data
__unittest.put.u8 $0
exit
)",
        magix::u8{255}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.u8 1
@entry:
load.u8 $0, #data
__unittest.put.u8 $0
exit
)",
        magix::u8{1}
    );
}
