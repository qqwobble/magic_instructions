#ifndef MAGIX_COMPILATION_PRINTING_HPP_
#define MAGIX_COMPILATION_PRINTING_HPP_

#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/ranges.hpp"
#include "magix_vm/variant_helper.hpp"

#include <codecvt>
#include <locale>
#include <ostream>
#include <variant>

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
    }
    // TODO: mark unreachable.
    return ostream;
}

inline std::ostream &
operator<<(std::ostream &ostream, const SrcLoc &loc)
{
    return ostream << loc.line << ':' << loc.column;
}

inline std::ostream &
print_srcsview(std::ostream &ostream, SrcView view)
{
    std::wstring_convert<std::codecvt_utf8<SrcChar>, SrcChar> cnv;
    auto *beg = view.data();
    auto *end = beg + view.size();
    return ostream << cnv.to_bytes(beg, end);
}

inline std::ostream &
operator<<(std::ostream &ostream, const SrcToken token)
{
    ostream << token.type << '[' << token.begin;
    if (token.begin != token.end)
    {
        ostream << '-' << token.end;
    }
    ostream << "]:\"";
    return print_srcsview(ostream, token.content) << '\"';
}

inline std::ostream &
operator<<(std::ostream &ostream, const AssemblerError &error)

{
    overload printer{
        [&ostream](const assembler_errors::NumberInvalid &err) -> auto & { return ostream << "IVALID_NUMBER:" << err.token; },
        [&ostream](const assembler_errors::NumberNotRepresentable &err) -> auto & { return ostream << "UNABLE_REPRESENT:" << err.token; },
        [&ostream](const assembler_errors::UnexpectedToken &err) -> auto & {
            auto type_it = err.expected.begin();
            ostream << "EXPECTED:" << *type_it++;
            for (auto x : ranges::subrange(type_it, err.expected.end()))
            {
                ostream << '|' << x;
            }
            return ostream << "-GOT:" << err.got;
        },
        [&ostream](const assembler_errors::UnknownInstruction &err) -> auto & {
            return ostream << "UNKNOWN_INSTRUCTION:" << err.instruction_name;
        },
        [&ostream](const assembler_errors::DuplicateLabels &err) -> auto & {
            return ostream << "DUP_LABEL:FIRST" << err.first_declaration << ':' << err.second_declaration;
        },
        [&ostream](const assembler_errors::MissingArgument &err) -> auto & {
            ostream << "MISSING_ARG:";
            print_srcsview(ostream, err.mnenomic);
            ostream << ':' << err.reg_number << ':';
            print_srcsview(ostream, err.reg_name);
            return ostream << '@' << err.source_instruction;
        },
        [&ostream](const assembler_errors::TooManyArguments &err) -> auto & {
            ostream << "TOO_MANY_ARGS";
            print_srcsview(ostream, err.mnenomic);
            ostream << ':' << err.reg_number;
            ostream << ':' << err.additional_reg;
            return ostream << '@' << err.source_instruction;
        },

        [&ostream](const assembler_errors::ExpectedLocalGotImmediate &err) -> auto & {
            ostream << "WANT_LOCAL_BUT_IMM:";
            print_srcsview(ostream, err.mnenomic);
            ostream << ':' << err.reg_number << ':';
            print_srcsview(ostream, err.reg_name);
            ostream << ':' << err.mismatched;
            return ostream << '@' << err.source_instruction;
        },
        [&ostream](const assembler_errors::ExpectedImmediateGotLocal &err) -> auto & {
            ostream << "WANT_IMM_BUT_LOCAL:";
            print_srcsview(ostream, err.mnenomic);
            ostream << ':' << err.reg_number << ':';
            print_srcsview(ostream, err.reg_name);
            ostream << ':' << err.mismatched;
            return ostream << '@' << err.source_instruction;
        },
        [&ostream](const assembler_errors::EntryMustPointToCode &err) -> auto & {
            return ostream << "ENTRY_NOT_TO_CODE" << err.label_declaration;
        },
        [&ostream](const assembler_errors::UnknownDirective &err) -> auto & { return ostream << "UNKNOWN_DIRECTIVE" << err.directive; },
        [&ostream](const assembler_errors::CompilationTooBig &err) -> auto & {
            return ostream << "COMPILATION_TOO_BIG" << err.data_size << '/' << err.maximum;
        },
        [&ostream](const assembler_errors::UnresolvedLabel &err) -> auto & { return ostream << "UNRESOLVED_LABEL" << err.which; },
        [&ostream](const assembler_errors::InternalError &err) -> auto & { return ostream << "INTERNAL:" << err.line_number; },
    };

    return std::visit(printer, error);
}

} // namespace magix

#endif // MAGIX_COMPILATION_PRINTING_HPP_
