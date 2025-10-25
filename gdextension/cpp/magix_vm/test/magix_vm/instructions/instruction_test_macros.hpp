#include <doctest.h>

#include "godot_cpp/classes/ref.hpp"

#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixCastSlot.hpp"
#include "magix_vm/MagixVirtualMachine.hpp"
#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/execution/executor.hpp"
#include "magix_vm/ranges.hpp"
#include "magix_vm/unique_node.hpp"
#include "magix_vm/utility.hpp"

#define MAGIX_TEST_CASE_EXECUTE_COMPARE(_test_name, _code, ...)                                                                            \
    TEST_CASE(_test_name)                                                                                                                  \
    {                                                                                                                                      \
        ::godot::Ref<::magix::MagixAsmProgram> prog;                                                                                       \
        prog.instantiate();                                                                                                                \
        prog->set_asm_source(_code);                                                                                                       \
        auto errors = prog->get_raw_errors();                                                                                              \
        ::magix::ranges::empty_range<const ::magix::compile::AssemblerError> expected_errors;                                              \
        CHECK_RANGE_EQ(errors, expected_errors);                                                                                           \
        ::godot::Ref<magix::MagixByteCode> bc = prog->get_bytecode();                                                                      \
        if (!CHECK_NE(bc, nullptr))                                                                                                        \
        {                                                                                                                                  \
            return;                                                                                                                        \
        }                                                                                                                                  \
        auto *entr = bc->get_code().entry_points.find("entry");                                                                            \
        if (!CHECK_NE(entr, nullptr))                                                                                                      \
        {                                                                                                                                  \
            return;                                                                                                                        \
        }                                                                                                                                  \
                                                                                                                                           \
        ::magix::UniqueNode<::magix::MagixVirtualMachine> vm{memnew(::magix::MagixVirtualMachine)};                                        \
        ::magix::MagixCaster *caster = memnew(::magix::MagixCaster);                                                                       \
        ::magix::MagixCastSlot *slot = memnew(::magix::MagixCastSlot);                                                                     \
        vm->add_child(caster);                                                                                                             \
        caster->add_child(slot);                                                                                                           \
        slot->set_program(prog);                                                                                                           \
                                                                                                                                           \
        slot->cast_spell(vm.get(), "entry");                                                                                               \
        auto res = vm->run_with_result(0.016);                                                                                             \
        if (CHECK_EQ(res.test_records.size(), 1))                                                                                          \
        {                                                                                                                                  \
            auto expected_output = ::magix::make_std_array<::magix::execute::PrimitiveUnion>(__VA_ARGS__);                                 \
            CHECK_RANGE_EQ(res.test_records[0], expected_output);                                                                          \
        }                                                                                                                                  \
    }
