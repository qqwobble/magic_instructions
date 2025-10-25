#include "godot_cpp/classes/ref.hpp"
#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixCastSlot.hpp"
#include "magix_vm/MagixCaster.hpp"
#include "magix_vm/MagixVirtualMachine.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"
#include "magix_vm/unique_node.hpp"

TEST_SUITE("instructions/nonop")
{
    TEST_CASE("nonop")
    {
        // cant really test much for nop ...
        godot::Ref<magix::MagixAsmProgram> prog;
        prog.instantiate();
        prog->set_asm_source(UR"(
@entry:
nonop
)");
        auto errors = prog->get_raw_errors();
        magix::ranges::empty_range<const ::magix::compile::AssemblerError> expected_errors;
        CHECK_RANGE_EQ(errors, expected_errors);
        godot::Ref<magix::MagixByteCode> bc = prog->get_bytecode();
        if (!CHECK_NE(bc, nullptr))
        {
            return;
        }
        auto *entr = bc->get_code().entry_points.find("entry");
        if (!CHECK_NE(entr, nullptr))
        {
            return;
        }
        magix::UniqueNode<magix::MagixVirtualMachine> vm{memnew(magix ::MagixVirtualMachine)};
        magix::MagixCaster *caster = memnew(magix::MagixCaster);
        magix::MagixCastSlot *slot = memnew(magix::MagixCastSlot);
        vm->add_child(caster);
        caster->add_child(slot);
        slot->set_program(prog);
        slot->cast_spell(vm.get(), "entry");
        auto res = vm->run_with_result(0.016);
        if (CHECK_EQ(res.test_records.size(), 1))
        {
            auto expected_output = magix::make_std_array<::magix::execute::PrimitiveUnion>();
            CHECK_RANGE_EQ(res.test_records[0], expected_output);
        }
        // but we can test if no actual instruction was emitted!
        const magix::code_word invalid_op{};
        auto expect_code = magix::span_of(invalid_op);
        auto is_code = magix::span(bc->get_code().code).first<2>().reinterpret_resize<const magix::code_word>();
        CHECK_RANGE_EQ(expect_code, is_code);
    }
}
