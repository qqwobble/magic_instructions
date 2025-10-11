#include "magix_vm/MagixCaster.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/core/class_db.hpp"

void
magix::MagixCaster::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_available_mana"), &magix::MagixCaster::get_available_mana);
    godot::ClassDB::bind_method(godot::D_METHOD("set_available_mana", "mana"), &magix::MagixCaster::set_available_mana);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "mana"), "set_available_mana", "get_available_mana");

    godot::ClassDB::bind_method(godot::D_METHOD("get_max_mana"), &magix::MagixCaster::get_max_mana);
    godot::ClassDB::bind_method(godot::D_METHOD("set_max_mana", "mana"), &magix::MagixCaster::set_max_mana);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "max_mana"), "set_max_mana", "get_max_mana");

    GDVIRTUAL_BIND(_allocate_mana, "requested");

    godot::ClassDB::bind_method(godot::D_METHOD("try_consume_mana"), &magix::MagixCaster::try_consume_mana, "requested");
}

auto
magix::MagixCaster::try_consume_mana(magix::f32 requested) -> magix::f32
{
    if (_mana_available < requested)
    {
        _mana_available = 0.0f;
        return 0.0;
    }
    else
    {
        _mana_available -= requested;
        return requested;
    }
}
