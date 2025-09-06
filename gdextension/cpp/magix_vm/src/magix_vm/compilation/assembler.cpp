#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/instruction_data.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/flagset.hpp"
#include "magix_vm/macros.hpp"
#include "magix_vm/ranges.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"

#include <array>
#include <charconv>
#include <map>
#include <system_error>
#include <type_traits>
#include <vector>

#ifdef MAGIX_BUILD_TESTS
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"

#include <ostream>
#endif

namespace
{

struct UnresolvedLabel
{
    enum class Type
    {
        NORMAL,
        ENTRY_LABEL,
    };

    Type type;
    magix::SrcToken declaration;

    constexpr bool
    operator==(const UnresolvedLabel &rhs) const noexcept
    {
        return type == rhs.type && declaration == rhs.declaration;
    }

    constexpr bool
    operator!=(const UnresolvedLabel &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct SegmentPosition
{
    enum class Segment
    {
        UNBOUND,
        CODE,
        DATA,
    };
    Segment segment = Segment::UNBOUND;
    magix::u16 offset;

    constexpr bool
    operator==(const SegmentPosition &rhs) const noexcept
    {
        return segment == rhs.segment && offset == rhs.offset;
    }

    constexpr bool
    operator!=(const SegmentPosition &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

#ifdef MAGIX_BUILD_TESTS
std::ostream &
operator<<(std::ostream &ostream, const SegmentPosition &segment)
{
    switch (segment.segment)
    {
    case SegmentPosition::Segment::UNBOUND:
    {
        ostream << '?';
        break;
    }
    case SegmentPosition::Segment::CODE:
    {
        ostream << 'c';
        break;
    }
    case SegmentPosition::Segment::DATA:
    {
        ostream << 'd';
        break;
    }
    }
    return ostream << '[' << segment.offset << ']';
}
#endif

struct LabelData
{
    SegmentPosition offset;
    magix::SrcToken declaration;

    constexpr static LabelData
    unbound_declaration(magix::SrcToken declaration)
    {
        return {
            {
                SegmentPosition::Segment::UNBOUND,
                0,
            },
            declaration,
        };
    }

    constexpr bool
    operator==(const LabelData &rhs) const noexcept
    {
        return offset == rhs.offset && declaration == rhs.declaration;
    }

    constexpr bool
    operator!=(const LabelData &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

struct LinkerTask
{
    enum class Mode
    {
        OVERWRITE,
        ADD,
    };
    Mode mode;
    SegmentPosition offset;
    magix::SrcView label_name;

    constexpr bool
    operator==(const LinkerTask &rhs) const noexcept
    {
        return mode == rhs.mode && offset == rhs.offset && label_name == rhs.label_name;
    }

    constexpr bool
    operator!=(const LinkerTask &rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

#ifdef MAGIX_BUILD_TESTS
std::ostream &
operator<<(std::ostream &ostream, const LinkerTask &task)
{
    switch (task.mode)
    {
    case LinkerTask::Mode::OVERWRITE:
    case LinkerTask::Mode::ADD:
        ostream << '+';
    }
    magix::print_srcsview(ostream, task.label_name);
    return ostream << '@' << task.offset;
}
#endif

/** Tracks data to remap a single register as part of instruction remapping. That is the original token, added offset, etc. */
struct TrackRemapRegister
{
    enum class Type
    {
        UNUSED,
        LOCAL,
        IMMEDIATE_SET,
        IMMEDIATE_TOKEN,
    };
    Type type = Type::UNUSED;
    magix::u16 value;
    magix::u16 offset;
    magix::SrcToken label_declaration;

    constexpr bool
    operator==(const TrackRemapRegister &rhs) const
    {
        if (type != rhs.type)
        {
            return false;
        }
        if (type == Type::UNUSED)
        {
            return true;
        }
        return value == rhs.value && offset == rhs.offset && label_declaration == rhs.label_declaration;
    }

    constexpr bool
    operator!=(const TrackRemapRegister &rhs) const
    {
        return !(*this == rhs);
    }
};

struct TrackRemapInstruction
{
    magix::SrcToken root_instruction;
    magix::SrcView mnenomic;
    std::array<TrackRemapRegister, magix::MAX_REGISTERS_PER_INSTRUCTION> registers;
    bool user_generated = false;

    MAGIX_CONSTEXPR_CXX20("std array eq")
    bool
    operator==(const TrackRemapInstruction &rhs) const
    {
        return root_instruction == rhs.root_instruction && mnenomic == rhs.mnenomic && registers == rhs.registers &&
               user_generated == rhs.user_generated;
    }

    MAGIX_CONSTEXPR_CXX20("std array eq")
    bool
    operator!=(const TrackRemapInstruction &rhs) const
    {
        return !(*this == rhs);
    }
};

#ifdef MAGIX_BUILD_TESTS
std::ostream &
operator<<(std::ostream &ostream, const TrackRemapRegister &remap_reg)
{
    switch (remap_reg.type)
    {
    case TrackRemapRegister::Type::UNUSED:
    {
        return ostream << "UNUSED";
    }
    case TrackRemapRegister::Type::LOCAL:
    {
        return ostream << '$' << magix::to_signed(remap_reg.value) << '+' << remap_reg.offset;
    }
    case TrackRemapRegister::Type::IMMEDIATE_SET:
    {
        return ostream << '#' << remap_reg.value << '+' << remap_reg.offset;
    }
    case TrackRemapRegister::Type::IMMEDIATE_TOKEN:
    {
        return magix::print_srcsview(ostream << '[', remap_reg.label_declaration.content) << "]+" << remap_reg.offset;
    }
    }
    // TODO: unreachable
    return ostream;
}

std::ostream &
operator<<(std::ostream &ostream, const TrackRemapInstruction &remap_inst)
{
    if (remap_inst.user_generated)
    {
        ostream << "U:";
    }
    magix::print_srcsview(ostream, remap_inst.root_instruction.content);
    for (const auto &reg : remap_inst.registers)
    {
        ostream << ' ' << reg;
    }
    return ostream;
}
#endif

struct ParsePreparePseudoResult
{
    bool parse_ok;
    bool instruction_emitted;
};

struct EatTokenResult
{
    bool found;
    const magix::SrcToken &token;
};

struct Assembler
{
    using span_type = magix::span<const magix::SrcToken>;

    void
    reset_to_src(span_type tokens);

    void
    assemble(span_type tokens);

    void
    discard_remaining_line();

    EatTokenResult
    eat_token(magix::BitEnumSet<magix::TokenType> allowed_tokens);

    template <class T>
    [[nodiscard]] bool
    extract_number(const magix::SrcToken &token, T &out);

    void
    parse_program();
    bool
    parse_statement();
    bool
    parse_label_or_instruction();
    bool
    parse_entry();
    bool
    parse_directive();
    bool
    parse_label(bool is_entry);
    bool
    parse_instruction();
    ParsePreparePseudoResult
    parse_prepare_pseudo_instruction();
    bool
    type_check_instruction(const TrackRemapInstruction &inst, const magix::InstructionSpec &spec);
    void
    remap_emit_instruction();
    void
    emit_instruction(const TrackRemapInstruction &inst, const magix::InstructionSpec &spec);

    void
    point_unresolved_labels_to_code();

    span_type::iterator_type current_token;
    span_type::iterator_type end_token;

    std::vector<UnresolvedLabel> unresolved_labels;
    std::vector<magix::SrcView> entry_labels;
    std::map<magix::SrcView, LabelData> labels;

    std::vector<TrackRemapInstruction> remap_cache;

    std::vector<magix::AssemblerError> error_stack;

    std::vector<LinkerTask> linker_tasks;

    std::vector<std::byte> data_segment;
    std::vector<magix::u16> code_segment;
};
} // namespace

template <class T>
bool
Assembler::extract_number(const magix::SrcToken &token, T &out)
{
    // basically put number into a format std::from_chars expect
    // which means ascii and no +, no 0x, etc...
    // then just call std::from_chars
    // then get errors like we want

    magix::SrcView number_lit = token.content;
    std::vector<char> char_stack;
    char_stack.reserve(number_lit.size() * sizeof(magix::SrcChar));
    auto it = number_lit.begin();
    auto end = number_lit.end();

    if (it == end)
    {
        // plz no empty
        error_stack.push_back(
            magix::assembler_errors::NumberInvalid{
                token,
            }
        );
        return false;
    }

    int radix = 10;

    // eat sign
    if (*it == '+')
    {
        // std::from_chars does not want it
        ++it;
    }
    else if (*it == '-')
    {
        if constexpr (std::is_unsigned_v<T>)
        {
            // can not represent negative
            error_stack.push_back(
                magix::assembler_errors::NumberNotRepresentable{
                    token,
                }
            );
            return false;
        }
        else
        {
            // std::from_chars needs the -
            ++it;
            char_stack.push_back('-');
        }
    }
    if (it == end)
    {
        // only sign is not a number
        error_stack.push_back(
            magix::assembler_errors::NumberInvalid{
                token,
            }
        );
        return false;
    }

    // hex mode
    if (end - it >= 2)
    {
        if (it[0] == '0' && (it[1] == 'x' || it[1] == 'X'))
        {
            it += 2;
            radix = 16;
        }
    }

    // now only copy the rest, let std::from_chars handle it
    while (it != end)
    {
        magix::SrcChar in = *it++;
        if (in > 127)
        {
            error_stack.push_back(
                magix::assembler_errors::NumberInvalid{
                    token,
                }
            );
            return false;
        }
        else
        {
            char_stack.push_back(static_cast<char>(in));
        }
    }

    const char *num_begin = char_stack.data();
    const char *num_end = num_begin + char_stack.size();

    std::from_chars_result result;

    if constexpr (std::is_integral_v<T>)
    {
        // int type
        result = std::from_chars(num_begin, num_end, out, radix);
        if (result.ptr != num_end || result.ec != std::errc{})
        {
            // failed, but would double work?
            // if so, return not representable instead of other error
            std::chars_format fmt = radix == 10 ? std::chars_format::general : std::chars_format::hex;
            magix::f64 test;
            std::from_chars_result result_double = std::from_chars(num_begin, num_end, test, fmt);
            if (result_double.ptr == num_end && result_double.ec == std::errc{})
            {
                error_stack.push_back(
                    magix::assembler_errors::NumberNotRepresentable{
                        token,
                    }
                );
                return false;
            }
        }
        // ok, it also aint a double
        // standard handling
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        // float type
        std::chars_format fmt = radix == 10 ? std::chars_format::general : std::chars_format::hex;
        result = std::from_chars(num_begin, num_end, out, fmt);
    }
    else
    {
        static_assert(!sizeof(T), "invalid type");
    }

    if (result.ec == std::errc::result_out_of_range)
    {
        error_stack.push_back(
            magix::assembler_errors::NumberNotRepresentable{
                token,
            }
        );
        return false;
    }
    else if (result.ec != std::errc{} || result.ptr != num_end)
    {
        // other types of error
        // ptr is compared, becuase std::from_chars considers partially eating the number a success.
        error_stack.push_back(
            magix::assembler_errors::NumberInvalid{
                token,
            }
        );
        return false;
    }

    return true;
}

#ifdef MAGIX_BUILD_TESTS

#define MAGIXTEST_PARSE_SIMPLE(type, value)                                                                                                \
    SUBCASE(#type ":" #value)                                                                                                              \
    {                                                                                                                                      \
        Assembler assembler;                                                                                                               \
        magix::SrcView literal = U## #value;                                                                                               \
        magix::SrcToken num_token{                                                                                                         \
            magix::TokenType::NUMBER,                                                                                                      \
            magix::SrcLoc{0, 0},                                                                                                           \
            magix::SrcLoc{0, literal.size()},                                                                                              \
            literal,                                                                                                                       \
        };                                                                                                                                 \
        type a = -1;                                                                                                                       \
        bool ok = assembler.extract_number(num_token, a);                                                                                  \
        CHECK(ok);                                                                                                                         \
        const auto &errors = assembler.error_stack;                                                                                        \
        std::array<magix::AssemblerError, 0> expected;                                                                                     \
        CHECK_RANGE_EQ(errors, expected);                                                                                                  \
        CHECK_EQ(a, value);                                                                                                                \
    }
#define MAGIXTEST_PARSE_ERR(type, value, err_type)                                                                                         \
    SUBCASE(#type ":" #value)                                                                                                              \
    {                                                                                                                                      \
        Assembler assembler;                                                                                                               \
        magix::SrcView literal = U## #value;                                                                                               \
        magix::SrcToken num_token{                                                                                                         \
            magix::TokenType::NUMBER,                                                                                                      \
            magix::SrcLoc{0, 0},                                                                                                           \
            magix::SrcLoc{0, literal.size()},                                                                                              \
            literal,                                                                                                                       \
        };                                                                                                                                 \
        type a = -1;                                                                                                                       \
        bool ok = assembler.extract_number(num_token, a);                                                                                  \
        CHECK(!ok);                                                                                                                        \
        const auto &errors = assembler.error_stack;                                                                                        \
        magix::AssemblerError expected[] = {                                                                                               \
            err_type{                                                                                                                      \
                num_token,                                                                                                                 \
            },                                                                                                                             \
        };                                                                                                                                 \
        CHECK_RANGE_EQ(errors, expected);                                                                                                  \
    }

TEST_CASE("assembler:parse normal")
{
    MAGIXTEST_PARSE_SIMPLE(magix::i32, 0);
    MAGIXTEST_PARSE_SIMPLE(magix::u32, 0);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 0);
    MAGIXTEST_PARSE_SIMPLE(magix::i32, 1);
    MAGIXTEST_PARSE_SIMPLE(magix::u32, 1);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 1);
    MAGIXTEST_PARSE_SIMPLE(magix::i32, +1);
    MAGIXTEST_PARSE_SIMPLE(magix::u32, +1);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, +1);
    MAGIXTEST_PARSE_SIMPLE(magix::i32, -1);
    MAGIXTEST_PARSE_ERR(magix::u32, -1, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, -1);
}
TEST_CASE("assembler:parse hex")
{
    MAGIXTEST_PARSE_SIMPLE(magix::i32, 0x0);
    MAGIXTEST_PARSE_SIMPLE(magix::u32, 0x0);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 0x0);
    MAGIXTEST_PARSE_SIMPLE(magix::i32, 0x1);
    MAGIXTEST_PARSE_SIMPLE(magix::u32, 0x1);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 0x1);
    MAGIXTEST_PARSE_SIMPLE(magix::i32, +0x1);
    MAGIXTEST_PARSE_SIMPLE(magix::u32, +0x1);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, +0x1);
    MAGIXTEST_PARSE_SIMPLE(magix::i32, -0x1);
    MAGIXTEST_PARSE_ERR(magix::u32, -0x1, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, -0x1);
}
TEST_CASE("assembler:parse float")
{
    MAGIXTEST_PARSE_ERR(magix::i32, 0.5, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_ERR(magix::u32, 0.5, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 0.5);
    MAGIXTEST_PARSE_ERR(magix::i32, -0.5, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_ERR(magix::u32, -0.5, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, -0.5);
}
TEST_CASE("assembler:parse float-hex")
{
    MAGIXTEST_PARSE_ERR(magix::i32, 0x1p-1, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_ERR(magix::u32, 0x1p-1, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 0x1p-1);
    MAGIXTEST_PARSE_ERR(magix::i32, -0x1p-1, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_ERR(magix::u32, -0x1p-1, magix::assembler_errors::NumberNotRepresentable);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, -0x1p-1);
}

#endif

EatTokenResult
Assembler::eat_token(magix::BitEnumSet<magix::TokenType> allowed_tokens)
{
    if (allowed_tokens.contains(current_token->type))
    {
        return {
            true,
            *current_token++,
        };
    }
    else
    {
        error_stack.push_back(
            magix::assembler_errors::UnexpectedToken{
                *current_token,
                allowed_tokens,
            }
        );
        return {
            false,
            *current_token,
        };
    }
}

bool
Assembler::parse_label(bool is_entry)
{
    auto [has_label, label_decl] = eat_token(magix::TokenType::IDENTIFIER);
    if (!has_label)
    {
        return false;
    }
    magix::SrcView label_name = label_decl.content;

    auto [insert_it, did_insert] = labels.try_emplace(label_name, LabelData::unbound_declaration(label_decl));

    if (did_insert)
    {
        // label is fresh
        UnresolvedLabel::Type type = is_entry ? UnresolvedLabel::Type::ENTRY_LABEL : UnresolvedLabel::Type::NORMAL;
        unresolved_labels.push_back({
            type,
            label_decl,
        });

        if (is_entry)
        {
            entry_labels.push_back(label_name);
        }
    }
    else
    {
        // duplicate label
        error_stack.push_back(
            magix::assembler_errors::DuplicateLabels{
                insert_it->second.declaration,
                label_decl,
            }
        );
        // this is not a parser error, so continue!
    }

    auto [has_colon, _colon_token] = eat_token(magix::TokenType::LABEL_MARKER);
    return has_colon;
}

void
Assembler::point_unresolved_labels_to_code()
{
    const magix::u16 code_bytes = static_cast<magix::u16>(code_segment.size() * sizeof(magix::u16));
    for (const auto &label : unresolved_labels)
    {
        switch (label.type)
        {
        case UnresolvedLabel::Type::NORMAL:
        case UnresolvedLabel::Type::ENTRY_LABEL:
        {
            labels[label.declaration.content] = {
                {
                    SegmentPosition::Segment::CODE,
                    code_bytes,
                },
                label.declaration,
            };
            continue;
        }
        }
        // TODO unreachable!
    }

    unresolved_labels.clear();
}

ParsePreparePseudoResult
Assembler::parse_prepare_pseudo_instruction()
{
    point_unresolved_labels_to_code();
    // this prepares the pseudo instruction as written, with only minimal preparsing
    // checks if instruction exists and parses numerical immediate values, everything else is done later at remapping
    // if anything fails, the instrucion is not added to the instruction stack.

    ParsePreparePseudoResult result;
    result.parse_ok = true;
    result.instruction_emitted = true;

    TrackRemapInstruction initial;
    initial = {}; // reset necessary!
    initial.user_generated = true;
    initial.root_instruction = *current_token++;
    initial.mnenomic = initial.root_instruction.content;

    size_t expected_argcount = magix::MAX_REGISTERS_PER_INSTRUCTION;
    const magix::InstructionSpec *spec = magix::get_instruction_spec(initial.mnenomic);
    if (spec != nullptr)
    {
        expected_argcount = 0;
        for (; expected_argcount < magix::MAX_REGISTERS_PER_INSTRUCTION; ++expected_argcount)
        {
            if (spec->registers[expected_argcount].mode == magix::InstructionRegisterSpec::Mode::UNUSED)
            {
                break;
            }
        }
    }
    else
    {
        error_stack.push_back(
            magix::assembler_errors::UnknownInstruction{
                initial.root_instruction,
            }
        );
        result.instruction_emitted = false;
    }

    bool emitted_too_many_args = false;

    size_t reg_count = 0;
    while (true)
    {
        switch (current_token->type)
        {
        case magix::TokenType::LINE_END:
        {
            // no more arguments
            ++current_token;
            goto exit;
        }
        case magix::TokenType::IMMEDIATE_MARKER:
        {
            ++current_token;

            auto [has_reg, reg_name] = eat_token(magix::TokenType::NUMBER | magix::TokenType::IDENTIFIER);
            if (!has_reg)
            {
                result.parse_ok = false;
                result.instruction_emitted = false;
                // parser errors can not be recovered, so just quit
                goto exit;
            }

            if (reg_count >= expected_argcount)
            {
                if (!emitted_too_many_args)
                {
                    error_stack.push_back(
                        magix::assembler_errors::TooManyArguments{
                            initial.root_instruction,
                            initial.mnenomic,
                            reg_name,
                            (int)reg_count,
                        }
                    );
                }
                result.instruction_emitted = false;
                emitted_too_many_args = true;
                // we continue parsing
            }
            else if (reg_name.type == magix::TokenType::IDENTIFIER)
            {
                initial.registers[reg_count++] = {
                    TrackRemapRegister::Type::IMMEDIATE_TOKEN,
                    0,
                    0,
                    reg_name,
                };
            }
            else if (spec != nullptr)
            {
                // number, try to parse it
                magix::u16 value = 0;
                bool value_valid = false;

#define MGX_INSTPREP_EXT(_type)                                                                                                            \
    do                                                                                                                                     \
    {                                                                                                                                      \
        _type typed;                                                                                                                       \
        if (extract_number(reg_name, typed))                                                                                               \
        {                                                                                                                                  \
            value = typed;                                                                                                                 \
            value_valid = true;                                                                                                            \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            result.instruction_emitted = false;                                                                                            \
        }                                                                                                                                  \
    } while (false)

                switch (spec->registers[reg_count].type)
                {

                case magix::InstructionRegisterSpec::Type::U8:
                case magix::InstructionRegisterSpec::Type::B8:
                {
                    MGX_INSTPREP_EXT(magix::u8);
                    break;
                }
                case magix::InstructionRegisterSpec::Type::U16:
                case magix::InstructionRegisterSpec::Type::B16:
                {
                    MGX_INSTPREP_EXT(magix::u16);
                    break;
                }
                case magix::InstructionRegisterSpec::Type::I8:
                {
                    MGX_INSTPREP_EXT(magix::i8);
                    break;
                }
                case magix::InstructionRegisterSpec::Type::I16:
                {
                    MGX_INSTPREP_EXT(magix::i16);
                    break;
                }
                default:
                {
                    // our isa should never have other immediate types!
                    // technically this should be an unreachable point of code
                    error_stack.push_back(magix::assembler_errors::InternalError{__LINE__});
                    result.instruction_emitted = false;
                    goto exit;
                }
                }

                if (value_valid)
                {
                    initial.registers[reg_count++] = {
                        TrackRemapRegister::Type::IMMEDIATE_SET,
                        value,
                        0,
                        reg_name,
                    };
                }
            }

            if (current_token->type == magix::TokenType::COMMA)
            {
                ++current_token;
                continue;
            }
            else if (current_token->type == magix::TokenType::LINE_END)
            {
                ++current_token;
                goto exit;
            }
            else
            {
                error_stack.push_back(
                    magix::assembler_errors::UnexpectedToken{
                        *current_token,
                        magix::TokenType::COMMA | magix::TokenType::LINE_END,
                    }
                );
                result.parse_ok = false;
                result.instruction_emitted = false;
                goto exit;
            }
        }
        case magix::TokenType::REGISTER_MARKER:
        {
            ++current_token;
            auto [has_reg, reg_name] = eat_token(magix::TokenType::NUMBER /* TODO: do named locals | magix::TokenType::IDENTIFIER */);
            if (!has_reg)
            {
                result.parse_ok = false;
                result.instruction_emitted = false;
                goto exit;
            }

            if (reg_name.type == magix::TokenType::NUMBER)
            {
                if (reg_count >= expected_argcount)
                {
                    if (!emitted_too_many_args)
                    {
                        error_stack.push_back(
                            magix::assembler_errors::TooManyArguments{
                                initial.root_instruction,
                                initial.mnenomic,
                                reg_name,
                                (int)reg_count,
                            }
                        );
                    }
                    result.instruction_emitted = false;
                    emitted_too_many_args = true;
                    // we continue parsing
                }
                else
                {
                    magix::i16 reg_off;
                    if (extract_number(reg_name, reg_off))
                    {
                        initial.registers[reg_count++] = {
                            TrackRemapRegister::Type::LOCAL,
                            magix::u16(reg_off),
                            0,
                            reg_name,
                        };
                    }
                }
            }
            else
            {
                // TODO named locals
            }

            if (current_token->type == magix::TokenType::COMMA)
            {
                ++current_token;
                continue;
            }
            else if (current_token->type == magix::TokenType::LINE_END)
            {
                ++current_token;
                goto exit;
            }
            else
            {
                error_stack.push_back(
                    magix::assembler_errors::UnexpectedToken{
                        *current_token,
                        magix::TokenType::COMMA | magix::TokenType::LINE_END,
                    }
                );
                result.parse_ok = false;
                result.instruction_emitted = false;
                goto exit;
            }
        }
        default:
        {
            error_stack.push_back(
                magix::assembler_errors::UnexpectedToken{
                    *current_token,
                    magix::TokenType::LINE_END | magix::TokenType::IMMEDIATE_MARKER | magix::TokenType::REGISTER_MARKER,
                }
            );
            result.parse_ok = false;
            result.instruction_emitted = false;
            goto exit;
        }
        }
    }

exit:

    if (result.instruction_emitted)
    {
        remap_cache.push_back(initial);
    }

    return result;
}

#ifdef MAGIX_BUILD_TESTS

TEST_CASE("assembler/preppseudo no arg")
{
    Assembler assm;
    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {0, 0},
            {0, 1},
            U"nop",
        },
        {
            magix::TokenType::LINE_END,
            {0, 0},
            {1, 0},
            U"\n",
        },
    };
    assm.reset_to_src(tokens);
    auto res = assm.parse_prepare_pseudo_instruction();
    CHECK(res.parse_ok);
    CHECK(res.instruction_emitted);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {
        {
            tokens[0],
            U"nop",
            {{}},
            true,
        },
    };
    CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
}

TEST_CASE("assembler/preppseudo $4,")
{
    Assembler assm;
    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {0, 0},
            {0, 1},
            U"set_stack",
        },
        {
            magix::TokenType::REGISTER_MARKER,
            {0, 1},
            {0, 2},
            U"$",
        },
        {
            magix::TokenType::NUMBER,
            {0, 2},
            {0, 3},
            U"4",
        },
        {
            magix::TokenType::COMMA,
            {0, 3},
            {0, 4},
            U",",
        },
        {
            magix::TokenType::LINE_END,
            {0, 0},
            {1, 0},
            U"\n",
        },
    };
    assm.reset_to_src(tokens);
    auto res = assm.parse_prepare_pseudo_instruction();
    CHECK(res.parse_ok);
    CHECK(res.instruction_emitted);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
        U"set_stack",
        {{
            {
                TrackRemapRegister::Type::LOCAL,
                4,
                0,
                tokens[2],
            },
        }},
        true,
    }};
    CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
}

TEST_CASE("assembler/preppseudo $-4")
{
    Assembler assm;
    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {0, 0},
            {0, 1},
            U"set_stack",
        },
        {
            magix::TokenType::REGISTER_MARKER,
            {0, 1},
            {0, 2},
            U"$",
        },
        {
            magix::TokenType::NUMBER,
            {0, 2},
            {0, 3},
            U"-1",
        },
        {
            magix::TokenType::LINE_END,
            {0, 0},
            {1, 0},
            U"\n",
        },
    };
    assm.reset_to_src(tokens);
    auto res = assm.parse_prepare_pseudo_instruction();
    CHECK(res.parse_ok);
    CHECK(res.instruction_emitted);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
        U"set_stack",
        {{
            {
                TrackRemapRegister::Type::LOCAL,
                magix::u16(-1),
                0,
                tokens[2],
            },
        }},
        true,
    }};
    CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
}

TEST_CASE("assembler/preppseudo #1")
{
    Assembler assm;
    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {0, 0},
            {0, 1},
            U"stack_resize",
        },
        {
            magix::TokenType::IMMEDIATE_MARKER,
            {0, 1},
            {0, 2},
            U"#",
        },
        {
            magix::TokenType::NUMBER,
            {0, 2},
            {0, 3},
            U"1",
        },
        {
            magix::TokenType::COMMA,
            {0, 3},
            {0, 4},
            U",",
        },
        {
            magix::TokenType::LINE_END,
            {0, 0},
            {1, 0},
            U"\n",
        },
    };
    assm.reset_to_src(tokens);
    auto res = assm.parse_prepare_pseudo_instruction();
    CHECK(res.parse_ok);
    CHECK(res.instruction_emitted);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
        U"stack_resize",
        {{
            {
                TrackRemapRegister::Type::IMMEDIATE_SET,
                1,
                0,
                tokens[2],
            },
        }},
        true,
    }};
    CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
}

TEST_CASE("assembler/preppseudo #-1")
{
    Assembler assm;
    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {0, 0},
            {0, 1},
            U"stack_resize",
        },
        {
            magix::TokenType::IMMEDIATE_MARKER,
            {0, 1},
            {0, 2},
            U"#",
        },
        {
            magix::TokenType::NUMBER,
            {0, 2},
            {0, 3},
            U"-1",
        },
        {
            magix::TokenType::LINE_END,
            {0, 0},
            {1, 0},
            U"\n",
        },
    };
    assm.reset_to_src(tokens);
    auto res = assm.parse_prepare_pseudo_instruction();
    CHECK(res.parse_ok);
    CHECK(res.instruction_emitted);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
        U"stack_resize",
        {{
            {
                TrackRemapRegister::Type::IMMEDIATE_SET,
                magix::u16(-1),
                0,
                tokens[2],
            },
        }},
        true,
    }};
    CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
}

#endif

void
Assembler::emit_instruction(const TrackRemapInstruction &inst, const magix::InstructionSpec &spec)
{
    // THIS FUNCTION ASSUMES EVERYTHING HAS BEEN TYPE CHECKED PRIOR TO CALLING

    constexpr auto encode_len = 1 + magix::MAX_REGISTERS_PER_INSTRUCTION;
    std::array<magix::u16, encode_len> encode_buf{};
    auto encode_it = encode_buf.begin();
    *encode_it++ = spec.opcode;

    const size_t code_size_bytes = code_segment.size() * sizeof(magix::u16);

    for (auto reg_index : magix::ranges::num_range(magix::MAX_REGISTERS_PER_INSTRUCTION))
    {
        const TrackRemapRegister &mapped_data = inst.registers[reg_index];
        const magix::InstructionRegisterSpec &spec_data = spec.registers[reg_index];
        if (spec_data.mode == magix::InstructionRegisterSpec::Mode::UNUSED)
        {
            break;
        }
        switch (mapped_data.type)
        {

        case TrackRemapRegister::Type::UNUSED:
        {
            // type check should prevent this!
            error_stack.push_back(
                magix::assembler_errors::InternalError{
                    __LINE__,
                }
            );
            return;
        }
        case TrackRemapRegister::Type::LOCAL: // todo: do named locals
        case TrackRemapRegister::Type::IMMEDIATE_SET:
        {
            *encode_it++ = mapped_data.value + mapped_data.offset;
            break;
        }
        case TrackRemapRegister::Type::IMMEDIATE_TOKEN:
        {
            *encode_it++ = mapped_data.offset;
            LinkerTask task{
                LinkerTask::Mode::ADD,
                {
                    SegmentPosition::Segment::CODE,
                    static_cast<magix::u16>(code_size_bytes + (1 + reg_index) * sizeof(magix::u16)),
                },
                mapped_data.label_declaration.content,
            };
            linker_tasks.push_back(task);
        }
        }
    }

    code_segment.insert(code_segment.end(), encode_buf.begin(), encode_it);
}

bool
Assembler::type_check_instruction(const TrackRemapInstruction &inst, const magix::InstructionSpec &spec)
{
    // as the preperation now also contains check (just to parse immediates of the right type)
    // this function is mainly for me, le the dev, because this __should__ not happen with a properly
    // specified pseudo instruction ...
    bool ok = true;

    for (const auto reg_index : magix::ranges::num_range(magix::MAX_REGISTERS_PER_INSTRUCTION))
    {
        const auto &specdata = spec.registers[reg_index];
        const auto &remapped = inst.registers[reg_index];
        switch (specdata.mode)
        {
        case magix::InstructionRegisterSpec::Mode::UNUSED:
        {
            switch (remapped.type)
            {
            case TrackRemapRegister::Type::UNUSED:
            {
                // we don't expect anything and don't get anything -> ok
                break; // remapped mode
            }
            case TrackRemapRegister::Type::LOCAL:
            case TrackRemapRegister::Type::IMMEDIATE_SET:
            case TrackRemapRegister::Type::IMMEDIATE_TOKEN:
            {
                error_stack.push_back(
                    magix::assembler_errors::TooManyArguments{
                        inst.root_instruction,
                        inst.mnenomic,
                        remapped.label_declaration,
                        (int)reg_index,
                    }
                );
                ok = false;
                break; // remapped mode
            }
            }
            break; // spec mode
        }
        case magix::InstructionRegisterSpec::Mode::LOCAL:
        {
            switch (remapped.type)
            {
            case TrackRemapRegister::Type::UNUSED:
            {
                error_stack.push_back(
                    magix::assembler_errors::MissingArgument{
                        inst.root_instruction,
                        inst.mnenomic,
                        specdata.name,
                        (int)reg_index,
                    }
                );
                ok = false;
                break; // remapped
            }
            case TrackRemapRegister::Type::LOCAL:
            {
                // is what we expected;
                break; // remapped
            }
            case TrackRemapRegister::Type::IMMEDIATE_SET:
            case TrackRemapRegister::Type::IMMEDIATE_TOKEN:
            {
                error_stack.push_back(
                    magix::assembler_errors::ExpectedLocalGotImmediate{
                        inst.root_instruction,
                        inst.mnenomic,
                        specdata.name,
                        (int)reg_index,
                        remapped.label_declaration,
                    }
                );
                ok = false;
                break; // remapped
            }
            }
            break; // spec mode
        }
        case magix::InstructionRegisterSpec::Mode::IMMEDIATE:
            switch (remapped.type)
            {
            case TrackRemapRegister::Type::UNUSED:
            {
                error_stack.push_back(
                    magix::assembler_errors::MissingArgument{
                        inst.root_instruction,
                        inst.mnenomic,
                        specdata.name,
                        (int)reg_index,
                    }
                );
                ok = false;
                break; // remapped
            }
            case TrackRemapRegister::Type::LOCAL:
            {
                error_stack.push_back(
                    magix::assembler_errors::ExpectedImmediateGotLocal{
                        inst.root_instruction,
                        inst.mnenomic,
                        specdata.name,
                        (int)reg_index,
                        remapped.label_declaration,
                    }
                );
                ok = false;
                break; // remapped
            }
            case TrackRemapRegister::Type::IMMEDIATE_SET:
            case TrackRemapRegister::Type::IMMEDIATE_TOKEN:
                // is ok.
                break;
            }
            break;
        }
    }
    return ok;
}

void
Assembler::remap_emit_instruction()
{
    // remap_cache is in reverse order, otherwise push/pop would be at the front
    while (!remap_cache.empty())
    {
        TrackRemapInstruction current = remap_cache.back();
        remap_cache.pop_back();

        const magix::InstructionSpec *spec = magix::get_instruction_spec(current.mnenomic);

        if (!spec)
        {
            error_stack.push_back(
                magix::assembler_errors::UnknownInstruction{
                    current.root_instruction,
                }
            );
            continue;
        }

        if (!type_check_instruction(current, *spec))
        {
            // will stop executing this branch and hopefully not rethrow the same errors while still allowing for other errors.
            continue;
        };

        if (!spec->is_pseudo)
        {
            emit_instruction(current, *spec);
            continue;
        }

        // reverse order because otherwise we would need to push/pop front
        for (const magix::PseudoInstructionTranslation &translation : spec->pseudo_translations | magix::ranges::reverse_view)
        {
            TrackRemapInstruction emit{};
            emit.root_instruction = current.root_instruction;
            emit.mnenomic = translation.out_mnenomic;
            emit.user_generated = false;
            for (auto ind : magix::ranges::num_range(magix::MAX_REGISTERS_PER_INSTRUCTION))
            {
                switch (translation.remaps[ind].type)
                {

                case magix::InstructionRegisterRemap::RemapType::UNUSED:
                {
                    // default is unused
                    emit.registers[ind] = {};
                    break;
                }
                case magix::InstructionRegisterRemap::RemapType::COPY:
                {
                    emit.registers[ind] = current.registers[translation.remaps[ind].value];
                    emit.registers[ind].offset += translation.remaps[ind].offset;
                    break;
                }
                case magix::InstructionRegisterRemap::RemapType::FIXED_IMMEDIATE:
                {
                    emit.registers[ind] = {
                        TrackRemapRegister::Type::IMMEDIATE_SET,
                        translation.remaps[ind].value,
                        translation.remaps[ind].offset,
                        {
                            magix::TokenType::IDENTIFIER,
                            magix::SrcLoc::max(),
                            magix::SrcLoc::max(),
                            U"__internal",
                        },
                    };
                    break;
                }
                case magix::InstructionRegisterRemap::RemapType::FIXED_LOCAL:
                    emit.registers[ind] = {
                        TrackRemapRegister::Type::LOCAL,
                        translation.remaps[ind].value,
                        translation.remaps[ind].offset,
                        {
                            magix::TokenType::IDENTIFIER,
                            magix::SrcLoc::max(),
                            magix::SrcLoc::max(),
                            U"__internal",
                        },
                    };
                    break;
                }
            }

            remap_cache.push_back(emit);
        }
    }
}

bool
Assembler::parse_instruction()
{
    // try to get the pseudo instruction as written
    ParsePreparePseudoResult prepare = parse_prepare_pseudo_instruction();
    if (prepare.instruction_emitted)
    {
        // if we have that pseudeo instruction get actual instruction sequence and emit it
        remap_emit_instruction();
        return true;
    }
    return prepare.parse_ok;
}

bool
Assembler::parse_label_or_instruction()
{
    switch (current_token[1].type)
    {
    case magix::TokenType::LABEL_MARKER:
    {
        return parse_label(false);
    }
    default:
    {
        // NOTE: There are instructions without arguments! Anything might follow.
        // label markers ":" however are not valid prefixes.
        // So everything that isn't a colon must mean this is an instruction!
        return parse_instruction();
    }
    }
}

bool
Assembler::parse_directive()
{
    // TODO: implement
    return false;
}

bool
Assembler::parse_entry()
{
    // @ has been checked
    ++current_token;
    return parse_label(true);
}

bool
Assembler::parse_statement()
{
    switch (current_token->type)
    {
    case magix::TokenType::LINE_END:
    {
        return eat_token(magix::TokenType::LINE_END).found;
    }
    case magix::TokenType::IDENTIFIER:
    {
        return parse_label_or_instruction();
    }
    case magix::TokenType::ENTRY_MARKER:
    {
        return parse_entry();
    }
    case magix::TokenType::DIRECTIVE_MARKER:
    {
        return parse_directive();
    }
    default:
    {
        error_stack.push_back(
            magix::assembler_errors::UnexpectedToken{
                *current_token,
                magix::TokenType::LINE_END | magix::TokenType::IDENTIFIER | magix::TokenType::ENTRY_MARKER |
                    magix::TokenType::DIRECTIVE_MARKER,
            }
        );
        return false;
    }
    }
}

void
Assembler::discard_remaining_line()
{
    for (; current_token->type != magix::TokenType::LINE_END; ++current_token)
    {
    }
}

void
Assembler::parse_program()
{
    while (current_token != end_token)
    {
        if (!parse_statement())
        {
            discard_remaining_line();
        }
    }
}

void
Assembler::reset_to_src(magix::span<const magix::SrcToken> tokens)
{
    current_token = tokens.begin();
    end_token = tokens.end();

    unresolved_labels.clear();
    entry_labels.clear();
    labels.clear();

    remap_cache.clear();

    error_stack.clear();

    linker_tasks.clear();

    data_segment.clear();
    code_segment.clear();
}

void
Assembler::assemble(magix::span<const magix::SrcToken> tokens)
{
    // TODO: properly assert this
    // assert(tokens.back().type == magix::TokenType::END_OF_FILE);

    // reset
    reset_to_src(tokens);

    // parse, this will call back into every emit function
    parse_program();

    if (error_stack.empty())
    {
        // link();
    }
}

void
assemble(magix::span<const magix::SrcToken> tokens)
{
    Assembler assembler;
    assembler.assemble(tokens);
}

#ifdef MAGIX_BUILD_TESTS

TEST_CASE("assembler: add.u32.imm $32, $24, #128")
{
    Assembler assembler;

    const magix::InstructionSpec *spec = magix::get_instruction_spec(U"add.u32.imm");
    if (!CHECK_NE(spec, nullptr))
    {
        return;
    }

    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {},
            {},
            U"add.u32.imm",
        },
        {
            magix::TokenType::REGISTER_MARKER,
            {},
            {},
            U"$",
        },
        {
            magix::TokenType::NUMBER,
            {},
            {},
            U"32",
        },
        {
            magix::TokenType::COMMA,
            {},
            {},
            U",",
        },
        {
            magix::TokenType::REGISTER_MARKER,
            {},
            {},
            U"$",
        },
        {
            magix::TokenType::NUMBER,
            {},
            {},
            U"28",
        },
        {
            magix::TokenType::COMMA,
            {},
            {},
            U",",
        },
        {
            magix::TokenType::IMMEDIATE_MARKER,
            {},
            {},
            U"#",
        },
        {
            magix::TokenType::NUMBER,
            {},
            {},
            U"128",
        },
        {
            magix::TokenType::LINE_END,
            {},
            {},
            U"\0",
        }
    };

    assembler.reset_to_src(tokens);
    assembler.parse_program();

    magix::span<magix::AssemblerError> expected_errs; // empty
    CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

    CHECK(assembler.remap_cache.empty());

    magix::span<const magix::SrcView> expected_entries;
    CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

    magix::span<const std::pair<const magix::SrcView, LabelData>> expected_labels;
    CHECK_RANGE_EQ(assembler.labels, expected_labels);

    magix::span<const UnresolvedLabel> expected_unresolved;
    CHECK_RANGE_EQ(assembler.unresolved_labels, expected_unresolved);

    const magix::u16 expected_code[] = {spec->opcode, 32, 28, 128};
    CHECK_RANGE_EQ(assembler.code_segment, expected_code);

    magix::span<const std::byte> expected_data;
    CHECK_RANGE_EQ(assembler.data_segment, expected_data);

    magix::span<const LinkerTask> expected_linker;
    CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);
}

#endif
