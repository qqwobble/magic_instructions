#ifndef MAGIX_COMPILATION_ASSEMBLER_HPP_
#define MAGIX_COMPILATION_ASSEMBLER_HPP_

#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/flagset.hpp"
#include "magix_vm/span.hpp"
#include <variant>

namespace magix
{

namespace assembler_errors
{

struct NumberInvalid
{
    SrcToken token;

    constexpr auto
    operator==(const NumberInvalid &rhs) const noexcept -> bool
    {
        return token == rhs.token;
    }
    constexpr auto
    operator!=(const NumberInvalid &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct NumberNotRepresentable
{
    SrcToken token;
    // TODO: add literal type

    constexpr auto
    operator==(const NumberNotRepresentable &rhs) const noexcept -> bool
    {
        return token == rhs.token;
    }
    constexpr auto
    operator!=(const NumberNotRepresentable &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct UnexpectedToken
{
    SrcToken got;
    BitEnumSet<TokenType> expected;

    constexpr auto
    operator==(const UnexpectedToken &rhs) const noexcept -> bool
    {
        return got == rhs.got && expected == rhs.expected;
    }
    constexpr auto
    operator!=(const UnexpectedToken &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct UnknownInstruction
{
    SrcToken instruction_name;

    constexpr auto
    operator==(const UnknownInstruction &rhs) const noexcept -> bool
    {
        return instruction_name == rhs.instruction_name;
    }
    constexpr auto
    operator!=(const UnknownInstruction &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct DuplicateLabels
{
    SrcToken first_declaration;
    SrcToken second_declaration;

    constexpr auto
    operator==(const DuplicateLabels &rhs) const noexcept -> bool
    {
        return first_declaration == rhs.first_declaration && second_declaration == rhs.second_declaration;
    }

    constexpr auto
    operator!=(const DuplicateLabels &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct MissingArgument
{
    SrcToken source_instruction;
    SrcView mnenomic;
    SrcView reg_name;
    int reg_number;

    constexpr auto
    operator==(const MissingArgument &rhs) const noexcept -> bool
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && reg_name == rhs.reg_name &&
               reg_number == rhs.reg_number;
    }

    constexpr auto
    operator!=(const MissingArgument &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct TooManyArguments
{
    SrcToken source_instruction;
    SrcView mnenomic;
    SrcToken additional_reg;
    int reg_number;

    constexpr auto
    operator==(const TooManyArguments &rhs) const noexcept -> bool
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && additional_reg == rhs.additional_reg &&
               reg_number == rhs.reg_number;
    }

    constexpr auto
    operator!=(const TooManyArguments &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct ExpectedLocalGotImmediate
{
    SrcToken source_instruction;
    SrcView mnenomic;
    SrcView reg_name;
    int reg_number;
    SrcToken mismatched;

    constexpr auto
    operator==(const ExpectedLocalGotImmediate &rhs) const noexcept -> bool
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && reg_name == rhs.reg_name &&
               reg_number == rhs.reg_number && mismatched == rhs.mismatched;
    }

    constexpr auto
    operator!=(const ExpectedLocalGotImmediate &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct ExpectedImmediateGotLocal
{
    SrcToken source_instruction;
    SrcView mnenomic;
    SrcView reg_name;
    int reg_number;
    SrcToken mismatched;

    constexpr auto
    operator==(const ExpectedImmediateGotLocal &rhs) const noexcept -> bool
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && reg_name == rhs.reg_name &&
               reg_number == rhs.reg_number && mismatched == rhs.mismatched;
    }

    constexpr auto
    operator!=(const ExpectedImmediateGotLocal &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct EntryMustPointToCode
{
    SrcToken label_declaration;

    constexpr auto
    operator==(const EntryMustPointToCode &rhs) const noexcept -> bool
    {
        return label_declaration == rhs.label_declaration;
    }

    constexpr auto
    operator!=(const EntryMustPointToCode &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct UnknownDirective
{
    SrcToken directive;

    constexpr auto
    operator==(const UnknownDirective &rhs) const noexcept -> bool
    {
        return directive == rhs.directive;
    }

    constexpr auto
    operator!=(const UnknownDirective &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct CompilationTooBig
{
    size_t data_size;
    size_t maximum;

    constexpr auto
    operator==(const CompilationTooBig &rhs) const noexcept -> bool
    {
        return data_size == rhs.data_size && maximum == rhs.maximum;
    }

    constexpr auto
    operator!=(const CompilationTooBig &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct UnboundLabel
{
    SrcToken which;

    constexpr auto
    operator==(const UnboundLabel &rhs) const noexcept -> bool
    {
        return which == rhs.which && which == rhs.which;
    }

    constexpr auto
    operator!=(const UnboundLabel &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct ConfigRedefinition
{
    SrcToken redef;

    constexpr auto
    operator==(const ConfigRedefinition &rhs) const noexcept -> bool
    {
        return redef == rhs.redef && redef == rhs.redef;
    }

    constexpr auto
    operator!=(const ConfigRedefinition &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

struct InternalError
{
    size_t line_number;
    constexpr auto
    operator==(const InternalError &rhs) const noexcept -> bool
    {
        return false;
    }

    constexpr auto
    operator!=(const InternalError &rhs) const noexcept -> bool
    {
        return false;
    }
};

using variant_type = std::variant<
    NumberInvalid,
    NumberNotRepresentable,
    UnexpectedToken,
    UnknownInstruction,
    DuplicateLabels,
    MissingArgument,
    TooManyArguments,
    ExpectedLocalGotImmediate,
    ExpectedImmediateGotLocal,
    EntryMustPointToCode,
    UnknownDirective,
    CompilationTooBig,
    UnboundLabel,
    ConfigRedefinition,
    InternalError>;
}; // namespace assembler_errors

class AssemblerError : public assembler_errors::variant_type
{
    using assembler_errors::variant_type::variant_type;
};

[[nodiscard]] auto
assemble(magix::span<const magix::SrcToken> tokens, magix::ByteCodeRaw &out) -> std::vector<magix::AssemblerError>;

} // namespace magix

#endif // MAGIX_COMPILATION_ASSEMBLER_HPP_
