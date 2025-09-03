#ifndef MAGIX_COMPILATION_LEXER_HPP_
#define MAGIX_COMPILATION_LEXER_HPP_

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
    IDENTIFIER,
    NUMBER,
    STRING,
    // #
    IMMEDIATE_MARKER,
    // $
    REGISTER_MARKER,
    // :
    LABEL_MARKER,
    // @
    ENTRY_MARKER,
    COMMA,
    // .
    DIRECTIVE_MARKER,
    LINE_END,
    INVALID_CHAR,
    UNTERMINATED_STRING,
};

struct SrcLoc
{
    size_t line;
    size_t column;

    [[nodiscard]] constexpr static SrcLoc
    zero()
    {
        return {0, 0};
    }

    [[nodiscard]] constexpr static SrcLoc
    max()
    {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
    }

    [[nodiscard]] constexpr SrcLoc
    nextcolumn(size_t count = 1) const
    {
        return {line, column + count};
    }

    constexpr SrcLoc &
    advance_column(size_t count = 1)
    {
        column += count;
        return *this;
    }

    [[nodiscard]] constexpr SrcLoc
    newline() const
    {
        return {line + 1, 0};
    }

    constexpr SrcLoc &
    advance_newline()
    {
        ++line;
        column = 0;
        return *this;
    }

    constexpr bool
    operator==(const SrcLoc &other) const
    {
        return line == other.line && column == other.column;
    }

    constexpr bool
    operator!=(const SrcLoc &other) const
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

    constexpr bool
    operator==(const SrcToken &other) const
    {
        return type == other.type && begin == other.begin && end == other.end && content == other.content;
    }

    constexpr bool
    operator!=(const SrcToken &other) const
    {
        return !(*this == other);
    }
};

[[nodiscard]] std::vector<SrcToken>
lex(SrcView source);

} // namespace magix

#endif // MAGIX_COMPILATION_LEXER_HPP_
