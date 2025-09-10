#ifndef MAGIX_COMPILATION_INSTRUCTION_DATA_HPP_
#define MAGIX_COMPILATION_INSTRUCTION_DATA_HPP_

#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"

namespace magix
{

constexpr size_t MAX_REGISTERS_PER_INSTRUCTION = 8;

/** Specify register info */
struct InstructionRegisterSpec
{
    enum class Mode
    {
        /** instruction does not take this many arguments */
        UNUSED,
        /** register is relative to stack */
        LOCAL,
        /** register is immediate */
        IMMEDIATE,
    };
    enum class Type
    {
        UNDEFINED,
        U8,
        U16,
        U32,
        U64,
        I8,
        I16,
        I32,
        I64,
        B8,
        B16,
        B32,
        B64,
        F32,
        F64,
    };

    Mode mode = Mode::UNUSED;
    Type type = Type::UNDEFINED;
    SrcView name;
};

/** Specify how registers are remapped when resolving pseudoinstructions. */
struct InstructionRegisterRemap
{
    enum class RemapType
    {
        /** Resulting instruction does not need this register. */
        UNUSED,
        /** Resulting instruction copies whatever is in the value-th register of pseudoinstruction. Adds offset. */
        COPY,
        /** Resulting instruction has a fixed immediate value, stored in value.
         * Adds offset, mostly to be compatible with additional remapping. */
        FIXED_IMMEDIATE,
        /** Resulting instruction has a fixed local, stored in value. This will probably be never used. Also adds offset. */
        FIXED_LOCAL,
    };
    RemapType type = RemapType::UNUSED;
    u16 value;
    u16 offset;
};

/** Specify one of possibly multiple emitted instruction and how registers are remapped. */
struct PseudoInstructionTranslation
{
    SrcView out_mnenomic;
    InstructionRegisterRemap remaps[MAX_REGISTERS_PER_INSTRUCTION];
};

/** Full data for every instruction. */
struct InstructionSpec
{
    SrcView mnenomic;
    bool is_pseudo;
    code_word opcode;
    InstructionRegisterSpec registers[MAX_REGISTERS_PER_INSTRUCTION];
    /** Pseudo instructions get replaced by this bad boi list. */
    ranges::subrange<const PseudoInstructionTranslation *> pseudo_translations;
};

/** Given instruction name get spec, if it exists, else nullptr */
[[nodiscard]] auto
get_instruction_spec(SrcView instruction_name) -> const InstructionSpec *;

/** All instructions, in definition order. */
[[nodiscard]] auto
all_instruction_specs() noexcept -> span<const InstructionSpec>;

} // namespace magix

#endif // MAGIX_COMPILATION_INSTRUCTION_DATA_HPP_
