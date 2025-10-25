#include "magix_vm/execution/runner.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/MagixCaster.hpp"
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/types.hpp"
#include "magix_vm/unique_node.hpp"

#ifndef MAGIX_BUILD_TESTS
#error TEST FILE BUILT WITHOUT TESTS ENABLED
#endif

TEST_CASE("execution simple pages")
{
    magix::execute::ExecRunner runner;

    godot::Ref<magix::MagixAsmProgram> prog;
    prog.instantiate();
    prog->set_asm_source(UR"(
.shared_size 4
.fork_size 4
mana_amount:
.f32 16.0
@entry:
    load.f32 $12, #mana_amount
    __unittest.put.f32 $12
    allocate_mana $12, $12
    __unittest.put.f32 $12
    set.u32 $0, #0,
    set.u32 $4, #100,
    set.u32 $8, #200,
    fork.store $4, #0, #4
    shared.store $8, #0, #4
loop:
    set.u32 $0, #0,
    fork.load $4, #0, #4
    shared.load $8, #0, #4
    add.u32.imm $0, $0, #1
    add.u32.imm $4, $4, #1
    add.u32.imm $8, $8, #1
    fork.store $4, #0, #4
    shared.store $8, #0, #4
    __unittest.put.u32 $0
    __unittest.put.u32 $4
    __unittest.put.u32 $8
    yield_to #loop
)");

    using PUnion = magix::execute::PrimitiveUnion;

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

    auto caster = magix::make_unique_node<magix::MagixCaster>();

    runner.enqueue_cast_spell(caster.get(), prog->get_bytecode(), entr->value());
    {
        auto res = runner.run_all();
        if (CHECK_EQ(res.test_records.size(), 1))
        {

            const PUnion expected_records[] = {
                magix::f32{16.0}, magix::f32{16.0}, magix::u32{1}, magix::u32{101}, magix::u32{201},
            };
            CHECK_RANGE_EQ(res.test_records[0], expected_records);
        }
    }
    {
        auto res = runner.run_all();
        if (CHECK_EQ(res.test_records.size(), 1))
        {

            const PUnion expected_records[] = {
                magix::u32{1},
                magix::u32{102},
                magix::u32{202},
            };
            CHECK_RANGE_EQ(res.test_records[0], expected_records);
        }
    }
    {
        auto res = runner.run_all();
        if (CHECK_EQ(res.test_records.size(), 1))
        {

            const PUnion expected_records[] = {
                magix::u32{1},
                magix::u32{103},
                magix::u32{203},
            };
            CHECK_RANGE_EQ(res.test_records[0], expected_records);
        }
    }
    {
        auto res = runner.run_all();
        if (CHECK_EQ(res.test_records.size(), 1))
        {

            const PUnion expected_records[] = {
                magix::u32{1},
                magix::u32{104},
                magix::u32{204},
            };
            CHECK_RANGE_EQ(res.test_records[0], expected_records);
        }
    }
    runner.enqueue_cast_spell(caster.get(), prog->get_bytecode(), entr->value());
    {
        auto res = runner.run_all();
        if (CHECK_EQ(res.test_records.size(), 1))
        {
            const PUnion expected_records[] = {
                // part 1
                magix::u32{1},
                magix::u32{105},
                magix::u32{205},
                // init part 2
                magix::f32{16.0},
                magix::f32{16.0},
                // regular part 2
                magix::u32{1},
                magix::u32{101},
                magix::u32{201},
            };
            CHECK_RANGE_EQ(res.test_records[0], expected_records);
        }
    }
    {
        auto res = runner.run_all();
        if (CHECK_EQ(res.test_records.size(), 1))
        {
            const PUnion expected_records[] = {
                magix::u32{1}, magix::u32{106}, magix::u32{202}, magix::u32{1}, magix::u32{102}, magix::u32{203},
            };
            CHECK_RANGE_EQ(res.test_records[0], expected_records);
        }
    }
    {
        auto res = runner.run_all();
        if (CHECK_EQ(res.test_records.size(), 1))
        {
            const PUnion expected_records[] = {
                magix::u32{1}, magix::u32{107}, magix::u32{204}, magix::u32{1}, magix::u32{103}, magix::u32{205},
            };
            CHECK_RANGE_EQ(res.test_records[0], expected_records);
        }
    }
}
