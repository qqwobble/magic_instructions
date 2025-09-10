#ifndef MAGIX_COMPILATION_LEXER_HPP_
#define MAGIX_COMPILATION_LEXER_HPP_

#include "magix_vm/flagset.hpp"
#include <limits>
#include <string_view>
#include <vector>

namespace magix
{

using SrcChar = char32_t;
using SrcView = std::basic_string_view<SrcChar>;

constexpr SrcChar SYMBOL_COMMENT = ';';
constexpr SrcChar SYMBOL_IMMEDIATE = '#';
constexpr SrcChar SYMBOL_REGISTER = '$';
constexpr SrcChar SYMBOL_LABEL = ':';
constexpr SrcChar SYMBOL_ENTRY = '@';
constexpr SrcChar SYMBOL_DIRECTIVE = '.';
constexpr SrcChar SYMBOL_QUOTES = '"';
constexpr SrcChar SYMBOL_NEWLINE = '\n';
constexpr SrcChar SYMBOL_ESCAPE = '\\';
constexpr SrcChar SYMBOL_COMMA = ',';

enum class TokenType
{
    // label-names/instructions
    IDENTIFIER = 1 << 0,
    NUMBER = 1 << 1,
    STRING = 1 << 2,
    // #
    IMMEDIATE_MARKER = 1 << 3,
    // $
    REGISTER_MARKER = 1 << 4,
    // :
    LABEL_MARKER = 1 << 5,
    // @
    ENTRY_MARKER = 1 << 6,
    COMMA = 1 << 7,
    // .
    DIRECTIVE_MARKER = 1 << 8,
    LINE_END = 1 << 9,
    INVALID_CHAR = 1 << 10,
    UNTERMINATED_STRING = 1 << 11,
};

MAGIX_DECLARE_BIT_ENUM_OPS(TokenType)

struct SrcLoc
{
    size_t line;
    size_t column;

    [[nodiscard]] constexpr static auto
    zero() -> SrcLoc
    {
        return {0, 0};
    }

    [[nodiscard]] constexpr static auto
    max() -> SrcLoc
    {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
    }

    [[nodiscard]] constexpr auto
    nextcolumn(size_t count = 1) const -> SrcLoc
    {
        return {line, column + count};
    }

    constexpr auto
    advance_column(size_t count = 1) -> SrcLoc &
    {
        column += count;
        return *this;
    }

    [[nodiscard]] constexpr auto
    newline() const -> SrcLoc
    {
        return {line + 1, 0};
    }

    constexpr auto
    advance_newline() -> SrcLoc &
    {
        ++line;
        column = 0;
        return *this;
    }

    constexpr auto
    operator==(const SrcLoc &other) const -> bool
    {
        return line == other.line && column == other.column;
    }

    constexpr auto
    operator!=(const SrcLoc &other) const -> bool
    {
        return !(*this == other);
    }
};

struct SrcToken
{
    TokenType type;
    SrcLoc begin;
    SrcLoc end;
    SrcView content;

    constexpr auto
    operator==(const SrcToken &other) const -> bool
    {
        return type == other.type && begin == other.begin && end == other.end && content == other.content;
    }

    constexpr auto
    operator!=(const SrcToken &other) const -> bool
    {
        return !(*this == other);
    }
};

[[nodiscard]] auto
lex(SrcView source) -> std::vector<SrcToken>;

} // namespace magix

#endif // MAGIX_COMPILATION_LEXER_HPP_
