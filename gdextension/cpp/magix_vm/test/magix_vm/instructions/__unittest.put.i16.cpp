#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.i16")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0", UR"(
@entry:
set.i16 $0, #0
__unittest.put.i16 $0
exit
)",
        magix::i16{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 1", UR"(
@entry:
set.i16 $0, #1
__unittest.put.i16 $0
exit
)",
        magix::i16{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put -1", UR"(
@entry:
set.i16 $0, #-1
__unittest.put.i16 $0
exit
)",
        magix::i16{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0,-1,2,-3", UR"(
@entry:
set.i16 $0, #0
set.i16 $2, #-1
set.i16 $4, #2
set.i16 $6, #-3
__unittest.put.i16 $0
__unittest.put.i16 $2
__unittest.put.i16 $4
__unittest.put.i16 $6
exit
)",
        magix::i16{0}, magix::i16{-1}, magix::i16{2}, magix::i16{-3}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 1", UR"(
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
        "load-put -1", UR"(
data:
.i16 -1
@entry:
load.i16 $0, #data
__unittest.put.i16 $0
exit
)",
        magix::i16{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 0x7fff", UR"(
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
        "load-put -0x8000", UR"(
data:
.i16 -0x8000
@entry:
load.i16 $0, #data
__unittest.put.i16 $0
exit
)",
        magix::i16{-0x8000}
    );
}
