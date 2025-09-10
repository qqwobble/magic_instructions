#ifndef MAGIX_MAGIXBYTECODE_HPP_
#define MAGIX_MAGIXBYTECODE_HPP_

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/rb_map.hpp>

#include "magix_vm/global_declarations.hpp"

namespace magix
{

class MagixByteCode : public godot::RefCounted
{
    GDCLASS(MagixByteCode, godot::RefCounted)

  public:
    MagixByteCode() = default;
    ~MagixByteCode() override = default;

    [[nodiscard]] auto
    get_code() const -> const RawByteCode &
    {
        return bytecode;
    }

    [[nodiscard]] auto
    list_entry_points() const -> godot::Dictionary;

    [[nodiscard]] auto
    get_entry(const godot::String &symbol_name, byte_code_addr &out_entry) const -> bool;

    [[nodiscard]] auto
    get_bytes() const -> godot::PackedByteArray;

  protected:
    static void
    _bind_methods();

  private:
    RawByteCode bytecode;
    godot::RBMap<godot::String, byte_code_addr> entry_points;
};

} // namespace magix

#endif // MAGIX_MAGIXBYTECODE_HPP_
