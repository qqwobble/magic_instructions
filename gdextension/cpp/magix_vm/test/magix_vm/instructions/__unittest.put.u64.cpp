#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.u64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0", UR"(
@entry:
set.u64 $0, #0
__unittest.put.u64 $0
exit
)",
        magix::u64{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 1", UR"(
@entry:
set.u64 $0, #1
__unittest.put.u64 $0
exit
)",
        magix::u64{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0,1,2,3", UR"(
@entry:
set.u64 $0, #0
set.u64 $8, #1
set.u64 $16, #2
set.u64 $24, #3
__unittest.put.u64 $0
__unittest.put.u64 $8
__unittest.put.u64 $16
__unittest.put.u64 $24
exit
)",
        magix::u64{0}, magix::u64{1}, magix::u64{2}, magix::u64{3}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0xFFFF", UR"(
@entry:
set.u64 $0, #0xFFFF
__unittest.put.u64 $0
exit
)",
        magix::u64{0xffff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 1", UR"(
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
        "load-put 0xffffffffffffffff", UR"(
data:
.u64 0xffffffffffffffff
@entry:
load.u64 $0, #data
__unittest.put.u64 $0
exit
)",
        magix::u64{0xffffffffffffffff}
    );
}
