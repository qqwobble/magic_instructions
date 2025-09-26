#include "magix_vm/execution/executor.hpp"
#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"

#ifndef MAGIX_BUILD_TESTS
#error TEST FILE BUILT WITHOUT TESTS ENABLED
#endif

TEST_CASE("exec addr_of")
{
    magix::SrcView source_code = UR"(
    ; just some data move entry to non trivial point
    . u8 23
@entry:
    addr_of $0, $1
    addr_of $8, $16
    ; test does not move further
)";
    auto tokens = magix::lex(source_code);
    magix::ByteCodeRaw bc;
    auto errors = magix::assemble(tokens, bc);
    magix::span<magix::AssemblerError> empty_errs;

    bool ok = true;
    ok &= CHECK_RANGE_EQ(errors, empty_errs);

    ok &= CHECK_EQ(bc.entry_points["entry"], 2);

    if (!ok)
    {
        return;
    }

    magix::ExecStack stack{};
    magix::PageInfo pages{
        &stack,
        magix::stack_size_default,
        magix::objbank_size_default,
    };

    magix::ExecutionContext context{
        0,
        nullptr,
    };

    magix::ExecResult res = magix::execute(bc, 2, pages, 2, context);

    auto is_byte = magix::span(stack.stack).as_const().first<16>();
    magix::u32 expect_u[4] = {
        1,
        0,
        16,
    };
    auto expect = magix::span(expect_u).as_bytes();
    CHECK_BYTESTRING_EQ(is_byte, expect);

    CHECK_EQ(res.type, magix::ExecResult::Type::TRAP_TOO_MANY_STEPS);
}

TEST_CASE("exec nop")
{
    magix::SrcView source_code = UR"(
    . u8 23
@entry:
    nop
)";
    auto tokens = magix::lex(source_code);
    magix::ByteCodeRaw bc;
    auto errors = magix::assemble(tokens, bc);
    magix::span<magix::AssemblerError> empty_errs;

    bool ok = true;
    ok &= CHECK_RANGE_EQ(errors, empty_errs);

    ok &= CHECK_EQ(bc.entry_points["entry"], 2);

    if (!ok)
    {
        return;
    }

    magix::ExecStack stack{};
    magix::PageInfo pages{
        &stack,
        magix::stack_size_default,
        magix::objbank_size_default,
    };

    magix::ExecutionContext context{
        0,
        nullptr,
    };

    magix::ExecResult res = magix::execute(bc, 2, pages, 1, context);

    auto is_byte = magix::span(stack.stack).as_const().first<16>();
    // nop shouldn't touch anything
    magix::u32 expect_u[4] = {};
    auto expect = magix::span(expect_u).as_bytes();
    CHECK_BYTESTRING_EQ(is_byte, expect);

    CHECK_EQ(res.type, magix::ExecResult::Type::TRAP_TOO_MANY_STEPS);
}

TEST_CASE("exec nonop")
{
    magix::SrcView source_code = UR"(
    . u8 23
@entry:
    nonop
)";
    auto tokens = magix::lex(source_code);
    magix::ByteCodeRaw bc;
    auto errors = magix::assemble(tokens, bc);
    magix::span<magix::AssemblerError> empty_errs;

    bool ok = true;
    ok &= CHECK_RANGE_EQ(errors, empty_errs);

    ok &= CHECK_EQ(bc.entry_points["entry"], 2);

    if (!ok)
    {
        return;
    }

    magix::ExecStack stack{};
    magix::PageInfo pages{
        &stack,
        magix::stack_size_default,
        magix::objbank_size_default,
    };

    magix::ExecutionContext context{
        0,
        nullptr,
    };

    magix::ExecResult res = magix::execute(bc, 2, pages, 1, context);

    auto is_byte = magix::span(stack.stack).as_const().first<16>();
    // nop shouldn't touch anything
    magix::u32 expect_u[4] = {};
    auto expect = magix::span(expect_u).as_bytes();
    CHECK_BYTESTRING_EQ(is_byte, expect);

    CHECK_EQ(res.type, magix::ExecResult::Type::TRAP_INVALID_INSTRUCTION);
}

TEST_CASE("exec load.u32")
{
    magix::SrcView source_code = UR"(
data0:
    .u32 123456
@entry:
    load.u32 $0, #data0
data1:
    .u32 777777
    load.u32 $4, #data1
)";
    auto tokens = magix::lex(source_code);
    magix::ByteCodeRaw bc;
    auto errors = magix::assemble(tokens, bc);
    magix::span<magix::AssemblerError> empty_errs;

    bool ok = true;
    ok &= CHECK_RANGE_EQ(errors, empty_errs);
    constexpr magix::u16 exp_entry = 8;
    ok &= CHECK_EQ(bc.entry_points["entry"], exp_entry);

    if (!ok)
    {
        return;
    }

    magix::ExecStack stack{};
    magix::PageInfo pages{
        &stack,
        magix::stack_size_default,
        magix::objbank_size_default,
    };

    magix::ExecutionContext context{
        0,
        nullptr,
    };

    magix::ExecResult res = magix::execute(bc, exp_entry, pages, 2, context);

    auto is_byte = magix::span(stack.stack).as_const().first<16>();
    // nop shouldn't touch anything
    magix::u32 expect_u[4] = {123456, 777777};
    auto expect = magix::span(expect_u).as_bytes();
    CHECK_BYTESTRING_EQ(is_byte, expect);

    CHECK_EQ(res.type, magix::ExecResult::Type::TRAP_TOO_MANY_STEPS);
}
