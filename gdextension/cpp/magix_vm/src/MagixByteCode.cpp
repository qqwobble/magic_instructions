#include "MagixByteCode.hpp"

void
magix::MagixByteCode::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("list_entry_points"), &MagixByteCode::list_entry_points);
}

godot::Dictionary
magix::MagixByteCode::list_entry_points() const
{
    godot::Dictionary output;
    for (auto [key, value] : entry_points)
    {
        output[key] = value;
    }
    return output;
}

bool
magix::MagixByteCode::get_entry(const godot::String &symbol_name, byte_code_addr &out_entry) const
{
    auto *elem = entry_points.find(symbol_name);
    if (elem != nullptr)
    {
        out_entry = elem->value();
        return true;
    }
    return false;
}

godot::PackedByteArray
magix::MagixByteCode::get_bytes() const
{
    godot::PackedByteArray ret;
    godot::Error err = static_cast<godot::Error>(ret.resize(bytecode.raw.size()));
    if (err != godot::Error::OK)
    {
        return ret;
    }
    std::memcpy(ret.ptrw(), bytecode.raw.data(), bytecode.raw.size());
    return ret;
}
