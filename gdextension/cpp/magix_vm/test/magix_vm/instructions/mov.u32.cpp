#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.u32")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov 0x01234567", UR"(
data:
.u32 0x01234567
@entry:
load.u32 $0, #data
__unittest.put.u32 $4
mov.u32 $4, $0
__unittest.put.u32 $4
exit
)",
        magix::u32{0}, magix::u32{0x01234567}
    );
}
