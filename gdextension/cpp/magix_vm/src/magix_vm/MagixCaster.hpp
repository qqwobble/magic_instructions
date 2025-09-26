#ifndef MAGIX_MAGIXCASTER_HPP_
#define MAGIX_MAGIXCASTER_HPP_

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/wrapped.hpp"

#include "magix_vm/execution/executor.hpp"

#include "types.hpp"
namespace magix
{

class MagixCaster : public godot::Node
{
    GDCLASS(MagixCaster, godot::Node)

  public:
    MagixCaster() = default;
    ~MagixCaster() = default;

    [[nodiscard]] auto
    get_available_mana() const -> magix::f32
    {
        return _mana_available;
    }
    void
    set_available_mana(magix::f32 mana)
    {
        _mana_available = mana;
    }

    [[nodiscard]] auto
    get_max_mana() const -> magix::f32
    {
        return _mana_max;
    }
    void
    set_max_mana(magix::f32 mana)
    {
        _mana_max = mana;
    }

    auto
    allocate_mana(magix::f32 requested) -> magix::f32;

  protected:
    static void
    _bind_methods();

  private:
    magix::f32 _mana_available = 1000.0;
    magix::f32 _mana_max = 1000.0;
};

} // namespace magix

#endif // MAGIX_MAGIXCASTER_HPP_
