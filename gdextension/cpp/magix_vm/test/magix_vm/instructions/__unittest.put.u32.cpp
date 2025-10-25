#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.u32")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "put 0", UR"(
@entry:
set.u32 $0, #0
__unittest.put.u32 $0
)",
        magix::u32{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "put 1", UR"(
@entry:
set.u32 $0, #1
__unittest.put.u32 $0
)",
        magix::u32{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "put 0,1,2,3", UR"(
@entry:
set.u32 $0, #0
set.u32 $4, #1
set.u32 $8, #2
set.u32 $12, #3
__unittest.put.u32 $0
__unittest.put.u32 $4
__unittest.put.u32 $8
__unittest.put.u32 $12
)",
        magix::u32{0}, magix::u32{1}, magix::u32{2}, magix::u32{3}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "put 0xFFFF", UR"(
@entry:
set.u32 $0, #0xFFFF
__unittest.put.u32 $0
)",
        magix::u32{0xffff}
    );
}
