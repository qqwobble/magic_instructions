#include <doctest.h>

#include "godot_cpp/core/memory.hpp"
#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixCastSlot.hpp"
#include "magix_vm/MagixCaster.hpp"
#include "magix_vm/MagixVirtualMachine.hpp"
#include "magix_vm/compilation/printing.hpp"

#ifndef MAGIX_BUILD_TESTS
#error TEST FILE INCLUDED IN NON TEST BUILD
#endif

TEST_CASE("setup with object_id")
{
    godot::Ref<magix::MagixAsmProgram> prog;
    prog.instantiate();
    prog->set_asm_source(UR"(
@entry:
get_caster $0
get_object_id $8, $0
put.u32 $0
put.u32 $4
put.u32 $8
put.u32 $12
put.u32 $16
put.u32 $20
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

    magix::MagixVirtualMachine *vm = memnew(magix::MagixVirtualMachine);
    magix::MagixCaster *caster = memnew(magix::MagixCaster);
    magix::MagixCastSlot *slot = memnew(magix::MagixCastSlot);
    vm->add_child(caster);
    caster->add_child(slot);
    slot->set_program(prog);

    slot->cast_spell(vm, "entry");
    vm->run(0.016);

    godot::memdelete(vm);
}
