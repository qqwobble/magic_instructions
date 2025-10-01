#ifndef MAGIX_MAGIXASMPROGRAM_HPP_
#define MAGIX_MAGIXASMPROGRAM_HPP_

#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/vector2i.hpp"
#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/compilation/assembler.hpp"

#include <godot_cpp/classes/resource.hpp>

namespace magix
{

#define MAGIX_ASM_PROGRAM_SIG_COMPILED "compiled"
#define MAGIX_ASM_PROGRAM_SIG_COMPILE_OK "compile_ok"
#define MAGIX_ASM_PROGRAM_SIG_COMPILE_FAILED "compile_fail"
#define MAGIX_ASM_PROGRAM_SIG_BYTECODE_INVALIDATED "bytecode_invalidated"

class MagixAsmProgram : public godot::Resource
{
    GDCLASS(MagixAsmProgram, godot::Resource)

  public:
    MagixAsmProgram() = default;
    ~MagixAsmProgram() override = default;

    [[nodiscard]] auto
    get_asm_source() const -> godot::String
    {
        return asm_source;
    }

    void
    set_asm_source(const godot::String &source_code);

    void
    reset();

    auto
    compile() -> bool;

    [[nodiscard]] auto
    get_bytecode() -> godot::Ref<magix::MagixByteCode>;

    [[nodiscard]] auto
    is_compilation_ok() -> bool
    {
        compile();
        return byte_code.is_valid();
    }

    [[nodiscard]] auto
    get_error_count() -> size_t;

    [[nodiscard]] auto
    get_error_info(size_t index) -> godot::Dictionary;

  protected:
    static void
    _bind_methods();

  private:
    godot::String asm_source;
    bool tried_compile = false;
    godot::Ref<magix::MagixByteCode> byte_code;
    std::vector<magix::compile::AssemblerError> errors;
};

} // namespace magix

#endif // MAGIX_MAGIXASMPROGRAM_HPP_
