#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.u8")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0x1337", UR"(
@entry:
set.u16 $0, #0x1337; little endian
__unittest.put.u8 $0 ; 37
__unittest.put.u8 $1 ; 13
exit
)",
        magix::u8{0x37}, magix::u8{0x13}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0xFEDC", UR"(
@entry:
set.u16 $0, #0xFEDC
__unittest.put.u8 $0
__unittest.put.u8 $1
exit
)",
        magix::u8{0xDC}, magix::u8{0xFE}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 1", UR"(
data:
.u8 1
@entry:
load.u8 $0, #data
__unittest.put.u8 $0
exit
)",
        magix::u8{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 255", UR"(
data:
.u8 255
@entry:
load.u8 $0, #data
__unittest.put.u8 $0
exit
)",
        magix::u8{255}
    );
}
