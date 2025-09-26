#ifndef MAGIX_COMPILATION_PRINTING_HPP_
#define MAGIX_COMPILATION_PRINTING_HPP_

#include "doctest.h"
#include "godot_cpp/templates/pair.hpp"
#include "godot_cpp/variant/string.hpp"
#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/convert_magix_godot.hpp"
#include "magix_vm/ranges.hpp"
#include "magix_vm/variant_helper.hpp"

#include "godot_cpp/classes/ref.hpp"

#include <codecvt>
#include <locale>
#include <ostream>
#include <variant>

namespace magix
{
inline auto
operator<<(std::ostream &ostream, const TokenType &type) -> std::ostream &
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

inline auto
operator<<(std::ostream &ostream, const SrcLoc &loc) -> std::ostream &
{
    return ostream << loc.line << ':' << loc.column;
}

inline auto
tostring_srcview(SrcView view) -> std::string
{
    std::wstring_convert<std::codecvt_utf8<SrcChar>, SrcChar> cnv;
    auto *beg = view.data();
    auto *end = beg + view.size();
    return cnv.to_bytes(beg, end);
}

inline auto
print_srcsview(std::ostream &ostream, SrcView view) -> std::ostream &
{
    return ostream << tostring_srcview(view);
}

inline auto
operator<<(std::ostream &ostream, const SrcToken token) -> std::ostream &
{
    ostream << token.type << '[' << token.begin;
    if (token.begin != token.end)
    {
        ostream << '-' << token.end;
    }
    ostream << "]:\"";
    return print_srcsview(ostream, token.content) << '\"';
}

inline auto
operator<<(std::ostream &ostream, const AssemblerError &error) -> std::ostream &
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
        [&ostream](const assembler_errors::UnboundLabel &err) -> auto & { return ostream << "UNBOUND_LABEL" << err.which; },
        [&ostream](const assembler_errors::ConfigRedefinition &err) -> auto & { return ostream << "REDIFINITION" << err.redef; },
        [&ostream](const assembler_errors::InternalError &err) -> auto & { return ostream << "INTERNAL:" << err.line_number; },
    };

    return std::visit(printer, error);
}

template <class T>
auto
operator<<(std::ostream &ostream, const godot::Ref<T> &ref) -> std::ostream &
{
    return ostream << ref.ptr();
}

} // namespace magix

namespace doctest
{
template <>
struct StringMaker<magix::SrcView>
{
    static auto
    convert(const magix::SrcView &view) -> String
    {
        std::string str = magix::tostring_srcview(view);
        return String{str.c_str(), (String::size_type)str.length()};
    }
};

template <class K, class V>
struct StringMaker<godot::KeyValue<K, V>>
{
    static auto
    convert(const godot::KeyValue<K, V> &kv) -> String
    {
        return toString(kv.key) + "->" + toString(kv.value);
    }
};

template <>
struct StringMaker<godot::String>
{
    static auto
    convert(const godot::String &str) -> String
    {
        return toString(magix::strview_from_godot(str));
    }
};
} // namespace doctest

#endif // MAGIX_COMPILATION_PRINTING_HPP_
