#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.i64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0", UR"(
@entry:
set.i64 $0, #0
__unittest.put.i64 $0
exit
)",
        magix::i64{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 1", UR"(
@entry:
set.i64 $0, #1
__unittest.put.i64 $0
exit
)",
        magix::i64{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put -1", UR"(
@entry:
set.i64 $0, #-1
__unittest.put.i64 $0
exit
)",
        magix::i64{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0,-1,2,-3", UR"(
@entry:
set.i64 $0, #0
set.i64 $8, #-1
set.i64 $16, #2
set.i64 $24, #-3
__unittest.put.i64 $0
__unittest.put.i64 $8
__unittest.put.i64 $16
__unittest.put.i64 $24
exit
)",
        magix::i64{0}, magix::i64{-1}, magix::i64{2}, magix::i64{-3}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 1", UR"(
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
        "load-put -1", UR"(
data:
.i64 -1
@entry:
load.i64 $0, #data
__unittest.put.i64 $0
exit
)",
        magix::i64{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 0x7fffffffffffffff", UR"(
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
        "load-put -0x8000000000000000", UR"(
data:
.i64 -0x8000000000000000
@entry:
load.i64 $0, #data
__unittest.put.i64 $0
exit
)",
        magix::i64{-0x7fffffffffffffff - 1}
    );
}
