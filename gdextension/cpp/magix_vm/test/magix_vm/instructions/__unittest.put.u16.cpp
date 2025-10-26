#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.u32")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0", UR"(
@entry:
set.u16 $0, #0
__unittest.put.u16 $0
exit
)",
        magix::u16{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 1", UR"(
@entry:
set.u16 $0, #1
__unittest.put.u16 $0
exit
)",
        magix::u16{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0,1,2,3", UR"(
@entry:
set.u16 $0, #0
set.u16 $2, #1
set.u16 $4, #2
set.u16 $6, #3
__unittest.put.u16 $0
__unittest.put.u16 $2
__unittest.put.u16 $4
__unittest.put.u16 $6
exit
)",
        magix::u16{0}, magix::u16{1}, magix::u16{2}, magix::u16{3}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0xFFFF", UR"(
@entry:
set.u16 $0, #0xFFFF
__unittest.put.u16 $0
exit
)",
        magix::u16{0xffff}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 1", UR"(
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
        "load-put 0xffff", UR"(
data:
.u16 0xffff
@entry:
load.u16 $0, #data
__unittest.put.u16 $0
exit
)",
        magix::u16{0xffff}
    );
}
