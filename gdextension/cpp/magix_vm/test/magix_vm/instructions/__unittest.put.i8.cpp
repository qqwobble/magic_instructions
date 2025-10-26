#include "magix_vm/instructions/instruction_test_macros.hpp"

TEST_SUITE("instructions/__unittest.put.i8")
{

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put 0x1337", UR"(
@entry:
set.u16 $0, #0x1337; little endian
__unittest.put.i8 $0 ; 37
__unittest.put.i8 $1 ; 13
exit
)",
        magix::i8{0x37}, magix::i8{0x13}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put -0x1337", UR"(
@entry:
; -0x1337 = ~0x1337 + 1 = 0xECC9 =0x(-14|-37)
set.u16 $0, #0xECC9
__unittest.put.i8 $0 ; -37
__unittest.put.i8 $1 ; -14
exit
)",
        magix::i8{-0x37}, magix::i8{-0x14}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "set-put -0x1337", UR"(
@entry:
; -0x1337 = ~0x1337 + 1 = 0xECC9 =0x(-14|-37)
set.u16 $0, #0xECC9
__unittest.put.i8 $0 ; -37
__unittest.put.i8 $1 ; -14
exit
)",
        magix::i8{-0x37}, magix::i8{-0x14}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 1", UR"(
data:
.i8 1
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put -1", UR"(
data:
.i8 -1
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{-1}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put 127", UR"(
data:
.i8 127
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{127}
    );

    MAGIX_TEST_CASE_EXECUTE_COMPARE(
        "load-put -128", UR"(
data:
.i8 -128
@entry:
load.i8 $0, #data
__unittest.put.i8 $0
exit
)",
        magix::i8{-128}
    );
}
