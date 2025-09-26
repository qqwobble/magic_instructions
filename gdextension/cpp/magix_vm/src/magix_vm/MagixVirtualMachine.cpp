#include "magix_vm/MagixVirtualMachine.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/execution/executor.hpp"

void
magix::MagixVirtualMachine::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("queue_execution", "bytecode", "entry", "caster"), &MagixVirtualMachine::queue_execution);
    godot::ClassDB::bind_method(godot::D_METHOD("run", "delta"), &MagixVirtualMachine::run);

#if MAGIX_BUILD_TESTS
    godot::ClassDB::bind_static_method("MagixVirtualMachine", godot::D_METHOD("run_tests"), &MagixVirtualMachine::run_tests);
#endif
}

auto
magix::MagixVirtualMachine::queue_execution(godot::Ref<MagixByteCode> bytecode, const godot::String entry, MagixCaster *caster) -> bool
{
    const ByteCodeRaw &bc = bytecode->get_code();
    auto find = bc.entry_points.find(entry);
    if (find == nullptr)
    {
        return false;
    }
    runner.enqueue_cast_spell(caster, std::move(bytecode), find->value());
    return true;
}

#if MAGIX_BUILD_TESTS

extern auto
magix_run_doctest() -> int;

auto
magix::MagixVirtualMachine::run_tests() -> int
{
    return magix_run_doctest();
}

#endif
