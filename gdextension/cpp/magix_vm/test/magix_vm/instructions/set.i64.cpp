#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/set.i64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 0", UR"(
@entry:
set.i64 $0, #0
__unittest.put.i64 $0
exit
)",
        magix::i64{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 255", UR"(
@entry:
set.i64 $0, #255
__unittest.put.i64 $0
exit
)",
        magix::i64{255}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set -1", UR"(
@entry:
set.i64 $0, #-1
__unittest.put.i64 $0
exit
)",
        magix::i64{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 0x7fff", UR"(
@entry:
set.i64 $0, #0x7fff
__unittest.put.i64 $0
exit
)",
        magix::i64{0x7fff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set -0x8000", UR"(
@entry:
set.i64 $0, #-0x8000
__unittest.put.i64 $0
exit
)",
        magix::i64{-0x8000}
    );
}
