#include "magix_vm/compare.hpp"
#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/compilation/config.hpp"
#include "magix_vm/compilation/instruction_data.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/ranges.hpp"
#include "magix_vm/types.hpp"

#include <algorithm>
#include <doctest.h>

namespace
{

auto
test_compile(const magix::compile::InstructionSpec &spec) -> bool
{
    std::array<magix::compile::SrcChar, 1024> test_buffer{};
    auto buffer_it = test_buffer.begin();
    auto buffer_end = test_buffer.end();
    // write mnenomic
    if (magix::safe_less(buffer_end - buffer_it, spec.mnenomic.size()))
    {
        FAIL("can't write mnenomic to buffer!");
        return false;
    }
    buffer_it = std::copy(spec.mnenomic.begin(), spec.mnenomic.end(), buffer_it);
    for (auto &&reg : spec.registers)
    {
        magix::compile::SrcView append;
        switch (reg.mode)
        {
        case magix::compile::InstructionRegisterSpec::Mode::UNUSED:
        {
            goto done_registers;
        }
        case magix::compile::InstructionRegisterSpec::Mode::LOCAL:
        {
            append = U" $0,";
            break;
        }
        case magix::compile::InstructionRegisterSpec::Mode::IMMEDIATE:
        {
            append = U" #0,";
        }
        }
        if (magix::safe_less(buffer_end - buffer_it, append.size()))
        {
            FAIL("can't append register to buffer!");
            return false;
        }
        buffer_it = std::copy(append.begin(), append.end(), buffer_it);
    }
done_registers:
    if (buffer_it == buffer_end)
    {
        FAIL("can't append /n to buffer!");
        return false;
    }
    *buffer_it++ = '\n';
    magix::compile::SrcView code{test_buffer.data(), static_cast<size_t>(buffer_it - test_buffer.begin())};
    CAPTURE(code);
    auto tokens = magix::compile::lex(code);
    magix::compile::ByteCodeRaw raw;
    auto errors = magix::compile::assemble(tokens, raw);
    magix::ranges::empty_range<magix::compile::AssemblerError> expect_error;
    return CHECK_RANGE_EQ(errors, expect_error);
}

} // namespace

TEST_SUITE("instructions/autotest")
{

    TEST_CASE("lookup by name")
    {
        for (auto &&spec : magix::compile::all_instruction_specs())
        {
            CAPTURE(spec.mnenomic);
            auto *lookup_spec = magix::compile::get_instruction_spec(spec.mnenomic);
            CHECK_EQ(&spec, lookup_spec);
        }
    }

    TEST_CASE("well formed")
    {
        for (auto &&spec : magix::compile::all_instruction_specs())
        {
            CAPTURE(spec.mnenomic);

            // test if opcode and pseudo are mutually exclusive
            bool valid_opcode = spec.opcode != magix::invalid_opcode;
            CHECK_NE(spec.is_pseudo, valid_opcode);

            // test that real instructions do not remap
            if (!spec.is_pseudo)
            {
                auto map_beg = spec.pseudo_translations.begin();
                auto map_end = spec.pseudo_translations.end();
                CHECK_EQ(map_beg, map_end);
            }

            // test args well formed
            bool has_seen_unused = false;
            for (auto &&reg : spec.registers)
            {
                constexpr static magix::compile::SrcView empty_name = U"";
                if (reg.mode == magix::compile::InstructionRegisterSpec::Mode::UNUSED)
                {
                    has_seen_unused = true;
                    CHECK_EQ(reg.name, empty_name);
                    CHECK_EQ(reg.type, magix::compile::InstructionRegisterSpec::Type::UNDEFINED);
                }
                else
                {
                    CHECK(!has_seen_unused);
                    CHECK_NE(reg.name, empty_name);
                    // not checking type as some actually don't have a type
                }
            }
        }
    }

    TEST_CASE("singular statement compile")
    {
        for (auto &&spec : magix::compile::all_instruction_specs())
        {
            test_compile(spec);
        }
    }
}
