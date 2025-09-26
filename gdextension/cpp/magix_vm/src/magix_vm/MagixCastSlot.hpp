#ifndef MAGIX_MAGIXCASTSLOT_HPP_
#define MAGIX_MAGIXCASTSLOT_HPP_

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/string.hpp"
#include "magix_vm/MagixAsmProgram.hpp"

namespace magix
{

class MagixVirtualMachine;

class MagixCastSlot : public godot::Node
{
    GDCLASS(MagixCastSlot, godot::Node)

  public:
    MagixCastSlot() = default;
    ~MagixCastSlot() = default;

    void
    set_program(godot::Ref<magix::MagixAsmProgram> program);
    [[nodiscard]] auto
    get_program() const -> godot::Ref<magix::MagixAsmProgram>;

    void
    set_caster_id(int64_t id)
    {
        caster_id = id;
    }
    [[nodiscard]] auto
    get_caster_id() const -> int64_t
    {
        return caster_id;
    }

    void
    cast_spell(MagixVirtualMachine *vm, const godot::String &entry);

    [[nodiscard]] auto
    _get_configuration_warnings() const -> godot::PackedStringArray override;

  protected:
    static void
    _bind_methods();

  private:
    void
    _program_updated();

  private:
    godot::Ref<magix::MagixAsmProgram> program;
    int64_t caster_id;
};
} // namespace magix

#endif // MAGIX_MAGIXCASTSLOT_HPP_
