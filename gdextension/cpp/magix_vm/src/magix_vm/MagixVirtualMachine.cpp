#include "magix_vm/MagixVirtualMachine.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "magix_vm/execution/executor.hpp"

void
magix::MagixVirtualMachine::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("reset_and_execute", "delta"), &MagixVirtualMachine::reset_and_execute);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_remaining", "delta"), &MagixVirtualMachine::execute_remaining);

    godot::ClassDB::bind_method(godot::D_METHOD("queue_execution", "program", "entry"), &MagixVirtualMachine::queue_execution);

#if MAGIX_BUILD_TESTS
    godot::ClassDB::bind_static_method("MagixVirtualMachine", godot::D_METHOD("run_tests"), &MagixVirtualMachine::run_tests);
#endif
}

void
magix::MagixVirtualMachine::queue_execution(godot::Ref<magix::MagixByteCode> program, const godot::String &entry)
{
    ERR_FAIL_NULL(program);
    const auto &bc = program->get_code();
    auto find = bc.entry_points.find(entry);
    if (find == nullptr)
    {
        return;
    }
    magix::u16 entry_addr = find->value();
    ExecStack stack;
    PageInfo page_info{&stack, sizeof(stack.stack)};
    godot::print_line("exec: ", (int)execute(bc, entry_addr, page_info, 100, nullptr).type);
}

void
magix::MagixVirtualMachine::reset_and_execute(float delta)
{
    // godot::print_error("todo");
}

void
magix::MagixVirtualMachine::execute_remaining(float delta)
{
    // godot::print_error("todo");
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
