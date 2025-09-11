#include "magix_vm/MagixByteCode.hpp"

void
magix::MagixByteCode::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("list_entry_points"), &MagixByteCode::list_entry_points);
    godot::ClassDB::bind_method(godot::D_METHOD("get_rom_bytes"), &MagixByteCode::get_rom_bytes);
}

auto
magix::MagixByteCode::list_entry_points() const -> godot::Dictionary
{
    godot::Dictionary output;
    for (auto [key, value] : bytecode.entry_points)
    {
        output[key] = value;
    }
    return output;
}

auto
magix::MagixByteCode::get_rom_bytes() const -> godot::PackedByteArray
{
    godot::PackedByteArray ret;
    godot::Error err = static_cast<godot::Error>(ret.resize(sizeof(bytecode.code)));
    if (err != godot::Error::OK)
    {
        return ret;
    }
    std::memcpy(ret.ptrw(), bytecode.code, sizeof(bytecode.code));
    return ret;
}
