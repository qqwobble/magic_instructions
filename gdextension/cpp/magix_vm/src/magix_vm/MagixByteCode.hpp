#ifndef MAGIX_MAGIXBYTECODE_HPP_
#define MAGIX_MAGIXBYTECODE_HPP_

#include "magix_vm/compilation/compiled.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/rb_map.hpp>

namespace magix
{

class MagixByteCode : public godot::RefCounted
{
    GDCLASS(MagixByteCode, godot::RefCounted)

  public:
    MagixByteCode() = default;
    ~MagixByteCode() override = default;

    [[nodiscard]] auto
    get_code() const -> const ByteCodeRaw &
    {
        return bytecode;
    }

    [[nodiscard]] auto
    get_code_write() -> ByteCodeRaw &
    {
        return bytecode;
    }

    [[nodiscard]] auto
    list_entry_points() const -> godot::Dictionary;

    [[nodiscard]] auto
    get_rom_bytes() const -> godot::PackedByteArray;

  protected:
    static void
    _bind_methods();

  private:
    ByteCodeRaw bytecode;
};

} // namespace magix

#endif // MAGIX_MAGIXBYTECODE_HPP_
