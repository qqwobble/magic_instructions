#ifndef MAGIX_MAGIXCASTNODE_HPP_
#define MAGIX_MAGIXCASTNODE_HPP_

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/string.hpp"
#include "magix_vm/MagixAsmProgram.hpp"

namespace magix
{

class MagixVirtualMachine;

class MagixCastNode : public godot::Node
{
    GDCLASS(MagixCastNode, godot::Node)

  public:
    MagixCastNode() = default;
    ~MagixCastNode() = default;

    void
    set_program(godot::Ref<magix::MagixAsmProgram> program);
    [[nodiscard]] auto
    get_program() const -> godot::Ref<magix::MagixAsmProgram>;

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
};
} // namespace magix

#endif // MAGIX_MAGIXCASTNODE_HPP_
