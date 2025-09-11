#include "magix_vm/MagixCastNode.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/core/error_macros.hpp"

#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/string.hpp"
#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/MagixVirtualMachine.hpp"
#include "magix_vm/compare.hpp"
#include "magix_vm/ranges.hpp"

void
magix::MagixCastNode::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_program"), &magix::MagixCastNode::get_program);
    godot::ClassDB::bind_method(godot::D_METHOD("set_program", "program"), &magix::MagixCastNode::set_program);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "program", godot::PROPERTY_HINT_RESOURCE_TYPE, "MagixAsmProgram"), "set_program",
        "get_program"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("cast_spell", "vm", "entry"), &magix::MagixCastNode::cast_spell);
}

void
magix::MagixCastNode::set_program(godot::Ref<magix::MagixAsmProgram> program)
{
#ifdef TOOLS_ENABLED
    godot::Callable callable = callable_mp(this, &MagixCastNode::_program_updated);
    if (this->program.is_valid())
    {
        this->program->disconnect("changed", callable);
    }
#endif

    this->program = program;

#ifdef TOOLS_ENABLED
    this->program->connect("changed", callable);
#endif

    _program_updated();
}

[[nodiscard]] auto
magix::MagixCastNode::get_program() const -> godot::Ref<magix::MagixAsmProgram>
{
    return this->program;
}

void
magix::MagixCastNode::_program_updated()
{
#ifdef TOOLS_ENABLED
    update_configuration_warnings();
#endif
}

void
magix::MagixCastNode::cast_spell(MagixVirtualMachine *vm, const godot::String &entry)
{
    ERR_FAIL_NULL(vm);
    if (!program.is_valid())
    {
        return;
    }
    if (!program->is_compilation_ok())
    {
        return;
    }
    godot::Ref<MagixByteCode> bc = program->get_bytecode();
    ERR_FAIL_NULL(bc);
    vm->queue_execution(bc, entry);
}

[[nodiscard]] auto
magix::MagixCastNode::_get_configuration_warnings() const -> godot::PackedStringArray
{
#ifdef TOOLS_ENABLED
    godot::PackedStringArray notify;
    if (!program.is_valid())
    {
        // TODO TRANSLATE???
        notify.push_back("No program!");
        return notify;
    }

    size_t err_c = program->get_error_count();
    for (auto err_i : ranges::num_range(magix::min(err_c, 5, safe_less)))
    {
        godot::Dictionary dict = program->get_error_info(err_i);
        godot::Array arrs;
        arrs.push_back(dict["type"]);
        arrs.push_back(dict["start_line"]);
        arrs.push_back(dict["start_column"]);
        arrs.push_back(dict["end_line"]);
        arrs.push_back(dict["end_column"]);
        notify.push_back(godot::String("{0}<{1}:{2}-{3}:{4}>").format(arrs));
    }

    return notify;

#else
    return godot::Node::_get_configuration_warnings();
#endif
}
