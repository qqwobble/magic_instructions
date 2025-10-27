#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/mov.u16")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "mov 0x0123", UR"(
data:
.u16 0x0123
@entry:
load.u16 $0, #data
__unittest.put.u16 $2
mov.u16 $2, $0
__unittest.put.u16 $2
exit
)",
        magix::u16{0}, magix::u16{0x0123}
    );
}
