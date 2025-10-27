#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.i8")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov -0x01", UR"(
data:
.i8 -0x01
@entry:
load.i8 $0, #data
__unittest.put.i8 $1
mov.i8 $1, $0
__unittest.put.i8 $1
exit
)",
        magix::i8{0}, magix::i8{-0x01}
    );
}
