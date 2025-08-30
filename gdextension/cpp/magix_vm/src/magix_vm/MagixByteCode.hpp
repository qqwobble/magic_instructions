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
    MagixByteCode() {}
    ~MagixByteCode() {}

    const RawByteCode &
    get_code() const
    {
        return bytecode;
    }

    [[nodiscard]] godot::Dictionary
    list_entry_points() const;

    [[nodiscard]] bool
    get_entry(const godot::String &symbol_name, byte_code_addr &out_entry) const;

    [[nodiscard]] godot::PackedByteArray
    get_bytes() const;

  protected:
    static void
    _bind_methods();

  private:
    RawByteCode bytecode;
    godot::RBMap<godot::String, byte_code_addr> entry_points;
};

} // namespace magix

#endif // MAGIX_MAGIXBYTECODE_HPP_
