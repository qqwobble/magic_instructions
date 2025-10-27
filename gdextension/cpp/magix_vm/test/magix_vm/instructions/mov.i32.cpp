#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.i32")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov -0x01234567", UR"(
data:
.i32 -0x01234567
@entry:
load.i32 $0, #data
__unittest.put.i32 $4
mov.i32 $4, $0
__unittest.put.i32 $4
exit
)",
        magix::i32{0}, magix::i32{-0x01234567}
    );
}
