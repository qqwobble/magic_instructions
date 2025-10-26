#include "godot_cpp/classes/ref.hpp"
#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixCastSlot.hpp"
#include "magix_vm/MagixCaster.hpp"
#include "magix_vm/MagixVirtualMachine.hpp"
#include "magix_vm/compilation/instruction_data.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/execution/executor.hpp"
#include "magix_vm/types.hpp"
#include "magix_vm/unique_node.hpp"

TEST_SUITE("instructions/nop")
{
    TEST_CASE("nop")
    {
        // cant really test much for nop ...
        godot::Ref<magix::MagixAsmProgram> prog;
        prog.instantiate();
        prog->set_asm_source(UR"(
@entry:
nop
exit
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
            auto expected_output = ::magix::make_std_array<::magix::execute::PrimitiveUnion>();
            CHECK_RANGE_EQ(res.test_records[0], expected_output);
        }
        // but we can test if the exact opcode was written
        auto spec_nop = magix::compile::get_instruction_spec(U"nop");
        if (!CHECK_NE(spec_nop, nullptr))
        {
            return;
        }
        if (!CHECK(!spec_nop->is_pseudo))
        {
            return;
        }
        auto spec_exit = magix::compile::get_instruction_spec(U"exit");
        if (!CHECK_NE(spec_exit, nullptr))
        {
            return;
        }
        const magix::code_word expected_code[] = {spec_nop->opcode, spec_exit->opcode};
        auto is_code = magix::span(bc->get_code().code).reinterpret_resize<const magix::code_word>().first<2>();
        CHECK_RANGE_EQ(expected_code, is_code);
    }
}
