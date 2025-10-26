#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/set.u64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 0", UR"(
@entry:
set.u64 $0, #0
__unittest.put.u64 $0
exit
)",
        magix::u64{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 255", UR"(
@entry:
set.u64 $0, #255
__unittest.put.u64 $0
exit
)",
        magix::u64{255}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 0xffff", UR"(
@entry:
set.u64 $0, #0xffff
__unittest.put.u64 $0
exit
)",
        magix::u64{0xffff}
    );
}
