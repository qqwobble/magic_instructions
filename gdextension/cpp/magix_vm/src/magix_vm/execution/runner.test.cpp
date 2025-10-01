#include "magix_vm/execution/runner.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"

#ifndef MAGIX_BUILD_TESTS
#error TEST FILE BUILT WITHOUT TESTS ENABLED
#endif

TEST_CASE("one two")
{
    magix::execute::ExecRunner runner;

    godot::Ref<magix::MagixAsmProgram> prog;
    prog.instantiate();
    prog->set_asm_source(UR"(
.shared_size 4
.fork_size 4
@entry:
    set.u32 $0, #0,
    set.u32 $4, #100,
    set.u32 $8, #200,
    fork.store $4, #0, #4
    shared.store $8, #0, #4
loop:
    fork.load $4, #0, #4
    shared.load $8, #0, #4
    add.u32.imm $0, $0, #1
    add.u32.imm $4, $4, #1
    add.u32.imm $8, $8, #1
    fork.store $4, #0, #4
    shared.store $8, #0, #4
    put.u32 $0
    put.u32 $4
    put.u32 $8
    yield_to #loop
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
    runner.enqueue_cast_spell(nullptr, prog->get_bytecode(), entr->value());
    runner.run_all();
    runner.run_all();
    runner.run_all();
    runner.run_all();
    runner.enqueue_cast_spell(nullptr, prog->get_bytecode(), entr->value());
    runner.run_all();
    runner.run_all();
    runner.run_all();
}
