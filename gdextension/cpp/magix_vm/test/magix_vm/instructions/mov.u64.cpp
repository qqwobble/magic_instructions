#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.u64")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov 0x0123456789ABCDEF", UR"(
data:
.u64 0x0123456789ABCDEF
@entry:
load.u64 $0, #data
__unittest.put.u64 $8
mov.u64 $8, $0
__unittest.put.u64 $8
exit
)",
        magix::u64{0}, magix::u64{0x0123456789ABCDEF}
    );
}
