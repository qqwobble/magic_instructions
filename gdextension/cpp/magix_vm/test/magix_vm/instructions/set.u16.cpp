#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/set.u16")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 0", UR"(
@entry:
set.u16 $0, #0
__unittest.put.u16 $0
exit
)",
        magix::u16{0}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 255", UR"(
@entry:
set.u16 $0, #255
__unittest.put.u16 $0
exit
)",
        magix::u16{255}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set 0xffff", UR"(
@entry:
set.u16 $0, #0xffff
__unittest.put.u16 $0
exit
)",
        magix::u16{0xffff}
    );
}
