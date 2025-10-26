#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/load.u64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 1", UR"(
data:
.u64 1
@entry:
load.u64 $0, #data
__unittest.put.u64 $0
exit
)",
        magix::u64{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load 0xffffffffffffffff", UR"(
data:
.u64 0xffffffffffffffff
@entry:
load.u64 $0, #data
__unittest.put.u64 $0
exit
)",
        magix::u64{0xffffffffffffffff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load ..|1", UR"(
.u8 0x77 ; don't load this!
data:
.u64 1
@entry:
load.u64 $0, #data
__unittest.put.u64 $0
exit
)",
        magix::u64{1}
    );
}
