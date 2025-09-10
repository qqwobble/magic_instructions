#include "magix_vm/MagixAsmProgram.hpp"

void
magix::MagixAsmProgram::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_asm_source"), &magix::MagixAsmProgram::get_asm_source);
    godot::ClassDB::bind_method(godot::D_METHOD("set_asm_source", "source_code"), &magix::MagixAsmProgram::set_asm_source, DEFVAL(false));
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "asm_source"), "set_asm_source", "get_asm_source");

    godot::ClassDB::bind_method(godot::D_METHOD("compile"), &magix::MagixAsmProgram::compile);

    godot::ClassDB::bind_method(godot::D_METHOD("get_byte_code"), &MagixAsmProgram::get_byte_code);
}

void
magix::MagixAsmProgram::set_asm_source(const godot::String &source_code)
{
    asm_source = source_code;
    did_compile = false;
    byte_code.unref();
}

void
magix::MagixAsmProgram::compile()
{
    did_compile = true;
    godot::print_line("todo, lol");
}

auto
magix::MagixAsmProgram::get_byte_code() -> godot::Ref<magix::MagixByteCode>
{
    if (did_compile)
    {
        return byte_code;
    }

    compile();
    return byte_code;
}
