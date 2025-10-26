#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/exit")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "exit", UR"(
@entry:
exit
__unittest.put.u32 $0 ; never executed
exit
)",
        // no output
    );
}
