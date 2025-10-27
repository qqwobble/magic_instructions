#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.i64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov -0x0123456789ABCDEF", UR"(
data:
.i64 -0x0123456789ABCDEF
@entry:
load.i64 $0, #data
__unittest.put.i64 $8
mov.i64 $8, $0
__unittest.put.i64 $8
exit
)",
        magix::i64{0}, magix::i64{-0x0123456789ABCDEF}
    );
}
