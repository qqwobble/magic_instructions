#ifndef MAGIX_MAGIXASMPROGRAM_HPP_
#define MAGIX_MAGIXASMPROGRAM_HPP_

#include "magix_vm/MagixByteCode.hpp"

#include <godot_cpp/classes/resource.hpp>

namespace magix
{

class MagixAsmProgram : public godot::Resource
{
    GDCLASS(MagixAsmProgram, godot::Resource)

  public:
    MagixAsmProgram() {}
    ~MagixAsmProgram() {}

    godot::String
    get_asm_source() const
    {
        return asm_source;
    }
    void
    set_asm_source(const godot::String &source_code);

    void
    compile();

    godot::Ref<magix::MagixByteCode>
    get_byte_code();

  protected:
    static void
    _bind_methods();

  private:
    godot::String asm_source;
    bool did_compile = false;
    godot::Ref<magix::MagixByteCode> byte_code;
};

} // namespace magix

#endif // MAGIX_MAGIXASMPROGRAM_HPP_
