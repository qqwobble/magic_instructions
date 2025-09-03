#ifndef MAGIX_COMPILATION_ASSEMBLER_HPP_
#define MAGIX_COMPILATION_ASSEMBLER_HPP_

#include "magix_vm/compilation/lexer.hpp"

namespace magix
{

struct AssemblerError
{
    enum class Type
    {
        NUMBER_INVALID,
        NUMBER_VALUE_NOT_REPRESENTABLE,
        UNEXPECTED_TOKEN,
        UNKNOWN_INSTRUCTION,
    };

    Type type;
    SrcToken main_token;

    constexpr bool
    operator==(const AssemblerError &rhs) const
    {
        return type == rhs.type && main_token == rhs.main_token;
    }

    constexpr bool
    operator!=(const AssemblerError &rhs) const
    {
        return !(*this == rhs);
    }
};

} // namespace magix

#endif // MAGIX_COMPILATION_ASSEMBLER_HPP_
