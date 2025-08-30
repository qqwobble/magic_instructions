#include "magix_vm/MagixVirtualMachine.hpp"

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
magix::MagixVirtualMachine::queue_execution(magix::MagixByteCode *program, const godot::String &entry)
{
    godot::print_error("todo");
}

void
magix::MagixVirtualMachine::reset_and_execute(float delta)
{
    godot::print_error("todo");
}

void
magix::MagixVirtualMachine::execute_remaining(float delta)
{
    godot::print_error("todo");
}

#if MAGIX_BUILD_TESTS

extern int
magix_run_doctest();

int
magix::MagixVirtualMachine::run_tests()
{
    return magix_run_doctest();
}

#endif
