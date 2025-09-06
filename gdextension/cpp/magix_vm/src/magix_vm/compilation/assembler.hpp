#ifndef MAGIX_COMPILATION_ASSEMBLER_HPP_
#define MAGIX_COMPILATION_ASSEMBLER_HPP_

#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/flagset.hpp"
#include <variant>

namespace magix
{

namespace assembler_errors
{

struct NumberInvalid
{
    SrcToken token;

    constexpr bool
    operator==(const NumberInvalid &rhs) const noexcept
    {
        return token == rhs.token;
    }
    constexpr bool
    operator!=(const NumberInvalid &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct NumberNotRepresentable
{
    SrcToken token;
    // TODO: add literal type

    constexpr bool
    operator==(const NumberNotRepresentable &rhs) const noexcept
    {
        return token == rhs.token;
    }
    constexpr bool
    operator!=(const NumberNotRepresentable &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct UnexpectedToken
{
    SrcToken got;
    BitEnumSet<TokenType> expected;

    constexpr bool
    operator==(const UnexpectedToken &rhs) const noexcept
    {
        return got == rhs.got && expected == rhs.expected;
    }
    constexpr bool
    operator!=(const UnexpectedToken &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct UnknownInstruction
{
    SrcToken instruction_name;

    constexpr bool
    operator==(const UnknownInstruction &rhs) const noexcept
    {
        return instruction_name == rhs.instruction_name;
    }
    constexpr bool
    operator!=(const UnknownInstruction &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct DuplicateLabels
{
    SrcToken first_declaration;
    SrcToken second_declaration;

    constexpr bool
    operator==(const DuplicateLabels &rhs) const noexcept
    {
        return first_declaration == rhs.first_declaration && second_declaration == rhs.second_declaration;
    }

    constexpr bool
    operator!=(const DuplicateLabels &rhs) const noexcept
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

    constexpr bool
    operator==(const MissingArgument &rhs) const noexcept
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && reg_name == rhs.reg_name &&
               reg_number == rhs.reg_number;
    }

    constexpr bool
    operator!=(const MissingArgument &rhs) const noexcept
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

    constexpr bool
    operator==(const TooManyArguments &rhs) const noexcept
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && additional_reg == rhs.additional_reg &&
               reg_number == rhs.reg_number;
    }

    constexpr bool
    operator!=(const TooManyArguments &rhs) const noexcept
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

    constexpr bool
    operator==(const ExpectedLocalGotImmediate &rhs) const noexcept
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && reg_name == rhs.reg_name &&
               reg_number == rhs.reg_number && mismatched == rhs.mismatched;
    }

    constexpr bool
    operator!=(const ExpectedLocalGotImmediate &rhs) const noexcept
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

    constexpr bool
    operator==(const ExpectedImmediateGotLocal &rhs) const noexcept
    {
        return source_instruction == rhs.source_instruction && mnenomic == rhs.mnenomic && reg_name == rhs.reg_name &&
               reg_number == rhs.reg_number && mismatched == rhs.mismatched;
    }

    constexpr bool
    operator!=(const ExpectedImmediateGotLocal &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct EntryMustPointToCode
{
    SrcToken label_declaration;

    constexpr bool
    operator==(const EntryMustPointToCode &rhs) const noexcept
    {
        return label_declaration == rhs.label_declaration;
    }

    constexpr bool
    operator!=(const EntryMustPointToCode &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct UnknownDirective
{
    SrcToken directive;

    constexpr bool
    operator==(const UnknownDirective &rhs) const noexcept
    {
        return directive == rhs.directive;
    }

    constexpr bool
    operator!=(const UnknownDirective &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct InternalError
{
    size_t line_number;
    constexpr bool
    operator==(const InternalError &rhs) const noexcept
    {
        return false;
    }

    constexpr bool
    operator!=(const InternalError &rhs) const noexcept
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
    InternalError>;
}; // namespace assembler_errors

class AssemblerError : public assembler_errors::variant_type
{
    using assembler_errors::variant_type::variant_type;
};

} // namespace magix

#endif // MAGIX_COMPILATION_ASSEMBLER_HPP_
