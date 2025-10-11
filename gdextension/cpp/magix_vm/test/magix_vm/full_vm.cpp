#include <doctest.h>

#include "godot_cpp/core/memory.hpp"
#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixCastSlot.hpp"
#include "magix_vm/MagixCaster.hpp"
#include "magix_vm/MagixVirtualMachine.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/unique_node.hpp"

#ifndef MAGIX_BUILD_TESTS
#error TEST FILE INCLUDED IN NON TEST BUILD
#endif

TEST_CASE("setup with object_id")
{
    godot::Ref<magix::MagixAsmProgram> prog;
    prog.instantiate();
    prog->set_asm_source(UR"(
@entry:
set.u32 $0, #7
get_caster $0
get_object_id $8, $0
__unittest.put.u32 $0
__unittest.put.u64 $8
)");

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

    magix::UniqueNode<magix::MagixVirtualMachine> vm{memnew(magix::MagixVirtualMachine)};
    magix::MagixCaster *caster = memnew(magix::MagixCaster);
    magix::MagixCastSlot *slot = memnew(magix::MagixCastSlot);
    vm->add_child(caster);
    caster->add_child(slot);
    slot->set_program(prog);

    slot->cast_spell(vm.get(), "entry");
    auto res = vm->run_with_result(0.016);
    if (CHECK_EQ(res.test_records.size(), 1))
    {
        const magix::execute::PrimitiveUnion expected_put[]{
            magix::u32{7},
            magix::u64{caster->get_instance_id()},
        };
        CHECK_RANGE_EQ(res.test_records[0], expected_put);
    }
}
