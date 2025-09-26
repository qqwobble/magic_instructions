#include "magix_vm/MagixCaster.hpp"

void
magix::MagixCaster::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_available_mana"), &magix::MagixCaster::get_available_mana);
    godot::ClassDB::bind_method(godot::D_METHOD("set_available_mana", "mana"), &magix::MagixCaster::set_available_mana);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "mana"), "set_available_mana", "get_available_mana");

    godot::ClassDB::bind_method(godot::D_METHOD("get_max_mana"), &magix::MagixCaster::get_max_mana);
    godot::ClassDB::bind_method(godot::D_METHOD("set_max_mana", "mana"), &magix::MagixCaster::set_max_mana);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "max_mana"), "set_max_mana", "get_max_mana");

    godot::ClassDB::bind_method(godot::D_METHOD("allocate_mana", "requested"), &magix::MagixCaster::allocate_mana);
}

auto
magix::MagixCaster::allocate_mana(magix::f32 requested) -> magix::f32
{
    return requested;
}
