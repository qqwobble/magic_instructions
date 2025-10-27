#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.u8")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov 0x01", UR"(
data:
.u8 0x01
@entry:
load.u8 $0, #data
__unittest.put.u8 $1
mov.u8 $1, $0
__unittest.put.u8 $1
exit
)",
        magix::u8{0}, magix::u8{0x01}
    );
}
