#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.i32")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0", UR"(
@entry:
set.i32 $0, #0
__unittest.put.i32 $0
exit
)",
        magix::i32{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 1", UR"(
@entry:
set.i32 $0, #1
__unittest.put.i32 $0
exit
)",
        magix::i32{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put -1", UR"(
@entry:
set.i32 $0, #-1
__unittest.put.i32 $0
exit
)",
        magix::i32{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0,-1,2,-3", UR"(
@entry:
set.i32 $0, #0
set.i32 $4, #-1
set.i32 $8, #2
set.i32 $12, #-3
__unittest.put.i32 $0
__unittest.put.i32 $4
__unittest.put.i32 $8
__unittest.put.i32 $12
exit
)",
        magix::i32{0}, magix::i32{-1}, magix::i32{2}, magix::i32{-3}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 1", UR"(
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
        "load-put -1", UR"(
data:
.i32 -1
@entry:
load.i32 $0, #data
__unittest.put.i32 $0
exit
)",
        magix::i32{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 0x7fffffff", UR"(
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
        "load-put -0x80000000", UR"(
data:
.i32 -0x80000000
@entry:
load.i32 $0, #data
__unittest.put.i32 $0
exit
)",
        magix::i32{-0x7fffffff - 1}
    );
}
