#ifndef MAGIX_COMPILATION_PRINTING_HPP_
#define MAGIX_COMPILATION_PRINTING_HPP_

#include "magix_vm/compilation/lexer.hpp"

#include <codecvt>
#include <locale>
#include <ostream>
#include <type_traits>

namespace magix
{
inline std::ostream &
operator<<(std::ostream &ostream, const TokenType &type)
{
    switch (type)
    {

    case TokenType::IDENTIFIER:
    {
        return ostream << "<IDENTIFIER>";
    }
    case TokenType::NUMBER:
    {
        return ostream << "<NUMBER>";
    }
    case TokenType::STRING:
    {
        return ostream << "<STRING>";
    }
    case TokenType::IMMEDIATE_MARKER:
    {
        return ostream << "<#>";
    }
    case TokenType::REGISTER_MARKER:
    {
        return ostream << "<$>";
    }
    case TokenType::LABEL_MARKER:
    {
        return ostream << "<:>";
    }
    case TokenType::ENTRY_MARKER:
    {
        return ostream << "<@>";
    }
    case TokenType::COMMA:
    {
        return ostream << "<,>";
    }
    case TokenType::DIRECTIVE_MARKER:
    {
        return ostream << "<.>";
    }
    case TokenType::LINE_END:
    {
        return ostream << "<LINE_END>";
    }
    case TokenType::INVALID_CHAR:
    {
        return ostream << "<INVALID>";
    }
    case TokenType::UNTERMINATED_STRING:
    {
        return ostream << "<STR-UNTERMINATED>";
    }
    default:
        return ostream << "<INVALID:" << static_cast<std::underlying_type_t<TokenType>>(type) << ">";
    }
}

inline std::ostream &
operator<<(std::ostream &ostream, const SrcLoc &loc)
{
    return ostream << loc.line << ':' << loc.column;
}

inline std::ostream &
operator<<(std::ostream &ostream, const SrcToken token)
{
    ostream << token.type << '[' << token.begin;
    if (token.begin != token.end)
    {
        ostream << '-' << token.end;
    }
    std::wstring_convert<std::codecvt_utf8<SrcChar>, SrcChar> cnv;
    auto *beg = token.content.data();
    auto *end = beg + token.content.size();
    return ostream << "]:\"" << cnv.to_bytes(beg, end) << '\"';
}

} // namespace magix

#endif // MAGIX_COMPILATION_PRINTING_HPP_
