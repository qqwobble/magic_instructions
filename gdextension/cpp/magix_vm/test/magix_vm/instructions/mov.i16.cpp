#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.i16")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov -0x0123", UR"(
data:
.i16 -0x0123
@entry:
load.i16 $0, #data
__unittest.put.i16 $2
mov.i16 $2, $0
__unittest.put.i16 $2
exit
)",
        magix::i16{0}, magix::i16{-0x0123}
    );
}
