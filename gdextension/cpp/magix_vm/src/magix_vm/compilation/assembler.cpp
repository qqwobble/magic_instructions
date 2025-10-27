#include "magix_vm/compilation/assembler.hpp"

#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/compilation/instruction_data.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/execution/config.hpp"
#include "magix_vm/flagset.hpp"
#include "magix_vm/macros.hpp"
#include "magix_vm/ranges.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"

#include "godot_cpp/variant/string.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <map>
#include <optional>
#include <system_error>
#include <type_traits>
#include <vector>

namespace
{

/** Stores information per label. */
struct LabelData
{
    enum class LabelMode
    {
        /** Will be attached later to anything. */
        UNBOUND_NORMAL,
        /** Will be attached later, must be code segment. */
        UNBOUND_ENTRY,
        /** Bound to data. */
        DATA,
        /** Bound to code. */
        CODE,
        /** Bound to an absolute value. */
        ABSOLUTE,
    };
    /** Is it (un)bound, and to what segment? */
    LabelMode mode;
    /** Offset relative to segment. */
    magix::u16 offset;
    magix::compile::SrcToken declaration;

    constexpr static auto
    unbound_declaration(magix::compile::SrcToken declaration, bool is_entry) -> LabelData
    {
        return {
            is_entry ? LabelMode::UNBOUND_ENTRY : LabelMode::UNBOUND_NORMAL,
            0,
            declaration,
        };
    }

    constexpr auto
    operator==(const LabelData &rhs) const noexcept -> bool
    {
        return mode == rhs.mode && offset == rhs.offset && declaration == rhs.declaration;
    }

    constexpr auto
    operator!=(const LabelData &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

/** Store locations to edit, once segments are done. */
struct LinkerTask
{
    enum class Mode
    {
        OVERWRITE,
        ADD,
    };
    enum class Segment
    {
        DATA,
        CODE,
    };

    Mode mode;
    Segment segment;
    magix::u16 offset;
    magix::compile::SrcToken label_token;

    constexpr auto
    operator==(const LinkerTask &rhs) const noexcept -> bool
    {
        return mode == rhs.mode && segment == rhs.segment && offset == rhs.offset && label_token == rhs.label_token;
    }

    constexpr auto
    operator!=(const LinkerTask &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }
};

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
    magix::compile::SrcToken label_declaration;

    constexpr auto
    operator==(const TrackRemapRegister &rhs) const -> bool
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

    constexpr auto
    operator!=(const TrackRemapRegister &rhs) const -> bool
    {
        return !(*this == rhs);
    }
};

/** Tracks data to remap a full instruction. That is the mnenomic andd an array of registers to remap. */
struct TrackRemapInstruction
{
    magix::compile::SrcToken root_instruction;
    magix::compile::SrcView mnenomic;
    std::array<TrackRemapRegister, magix::compile::MAX_REGISTERS_PER_INSTRUCTION> registers;
    bool user_generated = false;

    MAGIX_CONSTEXPR_CXX20("std array eq") auto
    operator==(const TrackRemapInstruction &rhs) const -> bool
    {
        return root_instruction == rhs.root_instruction && mnenomic == rhs.mnenomic && registers == rhs.registers &&
               user_generated == rhs.user_generated;
    }

    MAGIX_CONSTEXPR_CXX20("std array eq") auto
    operator!=(const TrackRemapInstruction &rhs) const -> bool
    {
        return !(*this == rhs);
    }
};

struct ParsePreparePseudoResult
{
    bool parse_ok;
    bool instruction_emitted;
};

struct EatTokenResult
{
    bool found;
    const magix::compile::SrcToken &token;
};

using ErrorStack = std::vector<magix::compile::AssemblerError>;

struct Assembler
{
    using span_type = magix::span<const magix::compile::SrcToken>;

    void
    reset_to_src(span_type tokens);

    /** While not line end, skip next token. Last token is always a line end, so this is safe. */
    void
    discard_remaining_line();

    /** Try to consume the next token, if type allows. Does NOT advance otherwise. */
    auto
    eat_token(magix::BitEnumSet<magix::compile::TokenType> allowed_tokens) -> EatTokenResult;

    void
    parse_program();
    auto
    parse_statement() -> bool;
    auto
    parse_label_or_instruction() -> bool;
    auto
    parse_entry() -> bool;
    template <class T>
    auto
    parse_config_value(std::optional<T> &out, const magix::compile::SrcToken &dir_tok) -> bool;
    template <class T>
    auto
    parse_data_directive() -> bool;
    auto
    parse_directive() -> bool;
    auto
    parse_label(bool is_entry) -> bool;
    auto
    parse_instruction() -> bool;
    auto
    parse_prepare_pseudo_instruction() -> ParsePreparePseudoResult;
    auto
    validate_instruction_args(const TrackRemapInstruction &inst, const magix::compile::InstructionSpec &spec) -> bool;
    void
    remap_emit_instruction();
    void
    emit_instruction(const TrackRemapInstruction &inst, const magix::compile::InstructionSpec &spec);

    void
    align_data_segment(size_t alignment);

    void
    bind_labels_to_data_segment();

    void
    bind_labels_to_code_segment();

    void
    link(magix::compile::ByteCodeRaw &code);

    span_type::iterator_type current_token;
    span_type::iterator_type end_token;

    std::vector<magix::compile::SrcToken> unbound_labels;
    std::vector<magix::compile::SrcView> entry_labels;
    std::map<magix::compile::SrcView, LabelData> labels;

    std::optional<magix::u32> stack_size;
    std::optional<magix::u32> fork_size;
    std::optional<magix::u32> shared_size;
    std::optional<magix::u32> obj_count;
    std::optional<magix::u32> obj_fork_count;
    std::optional<magix::u32> obj_shared_count;

    std::vector<TrackRemapInstruction> remap_cache;

    ErrorStack error_stack;

    std::vector<LinkerTask> linker_tasks;

    std::vector<std::byte> data_segment;
    std::vector<magix::code_word> code_segment;
};
} // namespace

template <class T>
[[nodiscard]] auto
extract_number(const magix::compile::SrcToken &token, T &out, ErrorStack &error_stack) -> bool
{
    // basically put number into a format std::from_chars expect
    // which means ascii and no +, no 0x, etc...
    // then just call std::from_chars
    // then get errors like we want

    magix::compile::SrcView number_lit = token.content;
    std::vector<char> char_stack;
    char_stack.reserve(number_lit.size() * sizeof(magix::compile::SrcChar));
    auto it = number_lit.begin();
    auto end = number_lit.end();

    if (it == end)
    {
        // plz no empty
        error_stack.emplace_back(
            magix::compile::assembler_errors::NumberInvalid{
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
        // std::from_chars needs the -
        ++it;
        char_stack.push_back('-');
    }
    if (it == end)
    {
        // only sign is not a number
        error_stack.emplace_back(
            magix::compile::assembler_errors::NumberInvalid{
                token,
            }
        );
        return false;
    }
    // at this point any leading +/- should be eaten
    if (*it == '-' || *it == '+')
    {
        error_stack.emplace_back(
            magix::compile::assembler_errors::NumberInvalid{
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
        magix::compile::SrcChar in = *it++;
        if (in > 127)
        {
            error_stack.emplace_back(
                magix::compile::assembler_errors::NumberInvalid{
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
                error_stack.emplace_back(
                    magix::compile::assembler_errors::NumberNotRepresentable{
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
        error_stack.emplace_back(
            magix::compile::assembler_errors::NumberNotRepresentable{
                token,
            }
        );
        return false;
    }
    else if (result.ec != std::errc{} || result.ptr != num_end)
    {
        // other types of error
        // ptr is compared, becuase std::from_chars considers partially eating the number a success.
        error_stack.emplace_back(
            magix::compile::assembler_errors::NumberInvalid{
                token,
            }
        );
        return false;
    }

    return true;
}

auto
Assembler::eat_token(magix::BitEnumSet<magix::compile::TokenType> allowed_tokens) -> EatTokenResult
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
        error_stack.emplace_back(
            magix::compile::assembler_errors::UnexpectedToken{
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

auto
Assembler::parse_label(bool is_entry) -> bool
{
    auto [has_label, label_decl] = eat_token(magix::compile::TokenType::IDENTIFIER);
    if (!has_label)
    {
        return false;
    }
    magix::compile::SrcView label_name = label_decl.content;

    auto [insert_it, did_insert] = labels.try_emplace(label_name, LabelData::unbound_declaration(label_decl, is_entry));

    if (did_insert) // label is fresh
    {
        unbound_labels.push_back(label_decl);

        if (is_entry)
        {
            entry_labels.push_back(label_name);
        }
    }
    else // duplicate label
    {
        error_stack.emplace_back(
            magix::compile::assembler_errors::DuplicateLabels{
                insert_it->second.declaration,
                label_decl,
            }
        );
        // this is not a parser error, so continue!
    }

    auto [has_colon, _colon_token] = eat_token(magix::compile::TokenType::LABEL_MARKER);
    return has_colon;
}

void
Assembler::bind_labels_to_data_segment()
{
    // get current offset in segment
    const magix::u16 data_bytes = static_cast<magix::u16>(data_segment.size());
    for (const auto &label : unbound_labels)
    {
        auto &data = labels[label.content];
        switch (data.mode)
        {
        case LabelData::LabelMode::UNBOUND_NORMAL:
        {
            // non-entry labels can bind to anything
            data.mode = LabelData::LabelMode::DATA;
            data.offset = data_bytes;
            break;
        }
        case LabelData::LabelMode::UNBOUND_ENTRY:
        {
            // entry labels only to code
            error_stack.emplace_back(
                magix::compile::assembler_errors::EntryMustPointToCode{
                    label,
                }
            );
            continue;
        }
        default:
        {
            MAGIX_UNREACHABLE("must be unbound!");
            break;
        }
        }
    }
    unbound_labels.clear();
}

void
Assembler::bind_labels_to_code_segment()
{
    // get current offset
    const magix::u16 code_bytes = static_cast<magix::u16>(code_segment.size() * magix::code_size_v<magix::code_word>);
    for (const auto &label : unbound_labels)
    {
        auto &data = labels[label.content];
        switch (data.mode)
        {
        case LabelData::LabelMode::UNBOUND_NORMAL:
        case LabelData::LabelMode::UNBOUND_ENTRY:
        {
            // both arbitrary and entry labels can bind to code.
            data.mode = LabelData::LabelMode::CODE;
            data.offset = code_bytes;
            break;
        }
        default:
        {
            MAGIX_UNREACHABLE("must be unbound!");
            break;
        }
        }
    }

    unbound_labels.clear();
}

auto
Assembler::parse_prepare_pseudo_instruction() -> ParsePreparePseudoResult
{
    bind_labels_to_code_segment();
    // this prepares the pseudo instruction as written, with only minimal preparsing
    // checks if instruction exists and parses numerical immediate values, everything else is done later at remapping
    // if anything fails, the instrucion is not added to the instruction stack.

    ParsePreparePseudoResult result{
        true,
        true,
    };

    TrackRemapInstruction initial{
        *current_token++,
        initial.root_instruction.content,
        {},
        true,
    };

    size_t expected_argcount = magix::compile::MAX_REGISTERS_PER_INSTRUCTION;
    const magix::compile::InstructionSpec *spec = magix::compile::get_instruction_spec(initial.mnenomic);
    if (spec != nullptr)
    {
        expected_argcount = spec->arg_count();
    }
    else
    {
        error_stack.emplace_back(
            magix::compile::assembler_errors::UnknownInstruction{
                initial.root_instruction,
            }
        );
        result.instruction_emitted = false;
        // the instruction not being present is not a parsing error, so we need to continue
    }

    // only do so once
    bool emitted_too_many_args = false;

    size_t reg_count = 0;
    while (true)
    {
        switch (current_token->type)
        {
        case magix::compile::TokenType::LINE_END:
        {
            // no more arguments
            ++current_token;
            goto exit;
        }
        case magix::compile::TokenType::IMMEDIATE_MARKER:
        {
            bool skip_parse = false;
            if (spec != nullptr && reg_count < expected_argcount)
            {
                if (spec->registers[reg_count].mode == magix::compile::InstructionRegisterSpec::Mode::LOCAL)
                {
                    error_stack.emplace_back(
                        magix::compile::assembler_errors::ExpectedLocalGotImmediate{
                            initial.root_instruction,
                            initial.mnenomic,
                            spec->registers[reg_count].name,
                            (int)reg_count,
                            *current_token,
                        }
                    );
                    // user gave us a local (that is a numerical offset to stackpointer) but we expected a (typed) immediate
                    // avoid reporting this error multiple times by skipping the parsing
                    skip_parse = true;
                }
            }

            ++current_token; // eat marker, now at label/value

            auto [has_reg, reg_name] = eat_token(magix::compile::TokenType::NUMBER | magix::compile::TokenType::IDENTIFIER);
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
                    error_stack.emplace_back(
                        magix::compile::assembler_errors::TooManyArguments{
                            initial.root_instruction,
                            initial.mnenomic,
                            reg_name,
                            (int)reg_count,
                        }
                    );
                }
                result.instruction_emitted = false;
                emitted_too_many_args = true;
                // not a parser error, continue parsing
            }
            else if (skip_parse)
            {
                // set value to silence follow on errors
                // note that skipping happens if type mismatches
                // so to avoid further errors, it is flipped as well
                initial.registers[reg_count++] = {
                    TrackRemapRegister::Type::LOCAL,
                    0,
                    0,
                    reg_name,
                };
            }
            // at this point everything should match up
            else if (reg_name.type == magix::compile::TokenType::IDENTIFIER)
            {
                // we gave a #label, resolved later by linker
                initial.registers[reg_count++] = {
                    TrackRemapRegister::Type::IMMEDIATE_TOKEN,
                    0,
                    0,
                    reg_name,
                };
            }
            else if (spec != nullptr)
            {
                // At this point it is a number, otherwise it would have failed earlier. Parse it.
                magix::u16 value = 0;
                bool value_valid = false;

#define MGX_PARSE_TYPED(_type)                                                                                                             \
    do                                                                                                                                     \
    {                                                                                                                                      \
        _type typed;                                                                                                                       \
        if (extract_number(reg_name, typed, error_stack))                                                                                  \
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

                case magix::compile::InstructionRegisterSpec::Type::U8:
                case magix::compile::InstructionRegisterSpec::Type::B8:
                {
                    MGX_PARSE_TYPED(magix::u8);
                    break;
                }
                case magix::compile::InstructionRegisterSpec::Type::U16:
                case magix::compile::InstructionRegisterSpec::Type::B16:
                {
                    MGX_PARSE_TYPED(magix::u16);
                    break;
                }
                case magix::compile::InstructionRegisterSpec::Type::I8:
                {
                    MGX_PARSE_TYPED(magix::i8);
                    break;
                }
                case magix::compile::InstructionRegisterSpec::Type::I16:
                {
                    MGX_PARSE_TYPED(magix::i16);
                    break;
                }
                default:
                {
                    // our isa should never have other immediate types!
                    // technically this should be an unreachable point of code
                    error_stack.emplace_back(magix::compile::assembler_errors::InternalError{__LINE__});
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

            // we expect comma or line end
            // if encountering comma, line end, the line end is handled by loop
            if (current_token->type == magix::compile::TokenType::COMMA)
            {
                ++current_token;
                continue;
            }
            else if (current_token->type == magix::compile::TokenType::LINE_END)
            {
                ++current_token;
                goto exit;
            }
            else
            {
                error_stack.emplace_back(
                    magix::compile::assembler_errors::UnexpectedToken{
                        *current_token,
                        magix::compile::TokenType::COMMA | magix::compile::TokenType::LINE_END,
                    }
                );
                result.parse_ok = false;
                result.instruction_emitted = false;
                goto exit;
            }
        }
        case magix::compile::TokenType::REGISTER_MARKER:
        {
            bool skip_parse = false;

            if (spec != nullptr && reg_count < expected_argcount)
            {
                if (spec->registers[reg_count].mode == magix::compile::InstructionRegisterSpec::Mode::IMMEDIATE)
                {
                    error_stack.emplace_back(
                        magix::compile::assembler_errors::ExpectedImmediateGotLocal{
                            initial.root_instruction,
                            initial.mnenomic,
                            spec->registers[reg_count].name,
                            (int)reg_count,
                            *current_token,
                        }
                    );
                    skip_parse = true;
                }
            }

            ++current_token;
            auto [has_reg, reg_name] =
                eat_token(magix::compile::TokenType::NUMBER /* TODO: do named locals | magix::compile::TokenType::IDENTIFIER */);
            if (!has_reg)
            {
                result.parse_ok = false;
                result.instruction_emitted = false;
                goto exit;
            }

            if (reg_name.type == magix::compile::TokenType::IDENTIFIER)
            {
                // TODO named locals
            }
            else if (skip_parse)
            {
                // set value to silence follow on errors
                // note that skipping happens if type mismatches
                // so to avoid further errors, it is flipped as well
                initial.registers[reg_count++] = {
                    TrackRemapRegister::Type::IMMEDIATE_SET,
                    0,
                    0,
                    reg_name,
                };
            }
            else if (reg_name.type == magix::compile::TokenType::NUMBER)
            {
                if (reg_count >= expected_argcount)
                {
                    if (!emitted_too_many_args)
                    {
                        error_stack.emplace_back(
                            magix::compile::assembler_errors::TooManyArguments{
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
                    if (extract_number(reg_name, reg_off, error_stack))
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

            if (current_token->type == magix::compile::TokenType::COMMA)
            {
                ++current_token;
                continue;
            }
            else if (current_token->type == magix::compile::TokenType::LINE_END)
            {
                ++current_token;
                goto exit;
            }
            else
            {
                error_stack.emplace_back(
                    magix::compile::assembler_errors::UnexpectedToken{
                        *current_token,
                        magix::compile::TokenType::COMMA | magix::compile::TokenType::LINE_END,
                    }
                );
                result.parse_ok = false;
                result.instruction_emitted = false;
                goto exit;
            }
        }
        default:
        {
            error_stack.emplace_back(
                magix::compile::assembler_errors::UnexpectedToken{
                    *current_token,
                    magix::compile::TokenType::LINE_END | magix::compile::TokenType::IMMEDIATE_MARKER |
                        magix::compile::TokenType::REGISTER_MARKER,
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

void
Assembler::emit_instruction(const TrackRemapInstruction &inst, const magix::compile::InstructionSpec &spec)
{
    // THIS FUNCTION ASSUMES EVERYTHING HAS BEEN TYPE CHECKED PRIOR TO CALLING

    constexpr auto encode_len = 1 + magix::compile::MAX_REGISTERS_PER_INSTRUCTION;
    std::array<magix::code_word, encode_len> encode_buf{};
    auto encode_it = encode_buf.begin();

    // copy opcode
    *encode_it++ = spec.opcode;

    const size_t code_size_bytes = code_segment.size() * magix::code_size_v<magix::code_word>;

    for (auto reg_index : magix::ranges::num_range(magix::compile::MAX_REGISTERS_PER_INSTRUCTION))
    {
        // then copy every register
        const TrackRemapRegister &mapped_data = inst.registers[reg_index];
        const magix::compile::InstructionRegisterSpec &spec_data = spec.registers[reg_index];
        if (spec_data.mode == magix::compile::InstructionRegisterSpec::Mode::UNUSED)
        {
            break;
        }
        switch (mapped_data.type)
        {
        case TrackRemapRegister::Type::UNUSED:
        {
            // prior checks should prevent an unused register ever being used with a value.
            error_stack.emplace_back(
                magix::compile::assembler_errors::InternalError{
                    __LINE__,
                }
            );
            return;
        }
        case TrackRemapRegister::Type::LOCAL: // todo: do named locals
        case TrackRemapRegister::Type::IMMEDIATE_SET:
        {
            // directly encoded registers are just value copied
            *encode_it++ = mapped_data.value + mapped_data.offset;
            break;
        }
        case TrackRemapRegister::Type::IMMEDIATE_TOKEN:
        {
            // labels must be linked later
            *encode_it++ = mapped_data.offset;
            LinkerTask task{
                LinkerTask::Mode::ADD,
                LinkerTask::Segment::CODE,
                static_cast<magix::u16>(code_size_bytes + (1 + reg_index) * magix::code_size_v<magix::code_word>),
                mapped_data.label_declaration,
            };
            linker_tasks.push_back(task);
        }
        }
    }

    code_segment.insert(code_segment.end(), encode_buf.begin(), encode_it);
}

auto
Assembler::validate_instruction_args(const TrackRemapInstruction &inst, const magix::compile::InstructionSpec &spec) -> bool
{
    // as the preperation now also contains check (just to parse immediates of the right type)
    // this function is mainly for me, le the dev, because this __should__ not happen with a properly
    // specified pseudo instruction ...
    bool ok = true;

    for (const auto reg_index : magix::ranges::num_range(magix::compile::MAX_REGISTERS_PER_INSTRUCTION))
    {
        const auto &specdata = spec.registers[reg_index];
        const auto &remapped = inst.registers[reg_index];
        switch (specdata.mode)
        {
        case magix::compile::InstructionRegisterSpec::Mode::UNUSED:
        {
            // per specification we expect nothing
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
                // expected nothing, got something -> err
                error_stack.emplace_back(
                    magix::compile::assembler_errors::TooManyArguments{
                        inst.root_instruction,
                        inst.mnenomic,
                        remapped.label_declaration,
                        (int)reg_index,
                    }
                );
                ok = false;
                break; // remapped type
            }
            }
            break; // spec mode
        }
        case magix::compile::InstructionRegisterSpec::Mode::LOCAL:
        {
            switch (remapped.type)
            {
            case TrackRemapRegister::Type::UNUSED:
            {
                // expected a local, but got nothing!
                error_stack.emplace_back(
                    magix::compile::assembler_errors::MissingArgument{
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
                // expected local, got register
                error_stack.emplace_back(
                    magix::compile::assembler_errors::ExpectedLocalGotImmediate{
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
        case magix::compile::InstructionRegisterSpec::Mode::IMMEDIATE:
            switch (remapped.type)
            {
            case TrackRemapRegister::Type::UNUSED:
            {
                // expected immediate, got nothing
                error_stack.emplace_back(
                    magix::compile::assembler_errors::MissingArgument{
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
                // expected immediate, got local
                error_stack.emplace_back(
                    magix::compile::assembler_errors::ExpectedImmediateGotLocal{
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

        const magix::compile::InstructionSpec *spec = magix::compile::get_instruction_spec(current.mnenomic);
        if (!spec)
        {
            magix::compile::SrcToken error_token = current.root_instruction;
            // TODO this is not ideal, there should be a proper indicator for erroring in internals...
            error_token.content = current.mnenomic;
            error_stack.emplace_back(
                magix::compile::assembler_errors::UnknownInstruction{
                    error_token,
                }
            );
            continue;
        }

        if (!validate_instruction_args(current, *spec))
        {
            // will stop executing this branch and hopefully not rethrow the same errors while still allowing for other errors.
            continue;
        };

        if (!spec->is_pseudo)
        {
            emit_instruction(current, *spec);
            continue;
        }
        // only pseudo instructions are remapped.

        // reverse order because otherwise we would need to push/pop front
        for (const magix::compile::PseudoInstructionTranslation &translation : spec->pseudo_translations | magix::ranges::reverse_view)
        {
            TrackRemapInstruction emit{
                current.root_instruction,
                translation.out_mnenomic,
                {},
                false,
            };
            for (auto ind : magix::ranges::num_range(magix::compile::MAX_REGISTERS_PER_INSTRUCTION))
            {
                switch (translation.remaps[ind].type)
                {
                case magix::compile::InstructionRegisterRemap::RemapType::UNUSED:
                {
                    emit.registers[ind] = {}; // default init means unused
                    break;
                }
                case magix::compile::InstructionRegisterRemap::RemapType::COPY:
                {
                    emit.registers[ind] = current.registers[translation.remaps[ind].value];
                    emit.registers[ind].offset += translation.remaps[ind].offset;
                    break;
                }
                case magix::compile::InstructionRegisterRemap::RemapType::FIXED_IMMEDIATE:
                {
                    // just write a fixed value
                    emit.registers[ind] = {
                        TrackRemapRegister::Type::IMMEDIATE_SET,
                        translation.remaps[ind].value,
                        translation.remaps[ind].offset,
                        {
                            magix::compile::TokenType::IDENTIFIER,
                            magix::compile::SrcLoc::max(),
                            magix::compile::SrcLoc::max(),
                            U"__internal",
                        },
                    };
                    break;
                }
                case magix::compile::InstructionRegisterRemap::RemapType::FIXED_LOCAL:
                {
                    // just write a fixed value
                    emit.registers[ind] = {
                        TrackRemapRegister::Type::LOCAL,
                        translation.remaps[ind].value,
                        translation.remaps[ind].offset,
                        {
                            magix::compile::TokenType::IDENTIFIER,
                            magix::compile::SrcLoc::max(),
                            magix::compile::SrcLoc::max(),
                            U"__internal",
                        },
                    };
                    break;
                }
                }
            }

            remap_cache.push_back(emit);
        }
        // all registers filled!
    }
    // all instructions mapped!
}

auto
Assembler::parse_instruction() -> bool
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

auto
Assembler::parse_label_or_instruction() -> bool
{
    switch (current_token[1].type)
    {
    case magix::compile::TokenType::LABEL_MARKER:
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

void
Assembler::align_data_segment(size_t alignment)
{
    const size_t is = data_segment.size();
    const size_t remainder = is % alignment;
    if (remainder != 0)
    {
        // not aligned, grow
        data_segment.resize(is + alignment - remainder, std::byte{});
    }
}

template <class T>
auto
Assembler::parse_data_directive() -> bool
{
    // type decl is already skipped!

    // doing this now, as any error should be cleared later
    align_data_segment(magix::code_align_v<T>);
    bind_labels_to_data_segment();

    T value;
    auto [has, number] = eat_token(magix::compile::TokenType::NUMBER);
    if (!has)
    {
        return false;
    }
    if (extract_number(number, value, error_stack))
    {
        // get raw buffer of size we __guarantee__ on all platforms
        std::byte raw[magix::code_size_v<T>] = {};
        // copy to the first bytes (usually this will be all bytes)
        // I don't think we'll ever have have type that must be widened to be consistent across platforms
        // I mean: we already use fixed size types
        std::memcpy(raw, &value, sizeof(value));
        data_segment.insert(data_segment.end(), magix::ranges::begin(raw), magix::ranges::end(raw));
    }

    return true;
}

template <class T>
auto
Assembler::parse_config_value(std::optional<T> &out, const magix::compile::SrcToken &dir_tok) -> bool
{
    auto [has, number] = eat_token(magix::compile::TokenType::NUMBER);
    if (!has)
    {
        return false;
    }
    magix::u32 config_value = 0;
    // always store something
    std::ignore = extract_number(number, config_value, error_stack);
    if (out.has_value())
    {
        error_stack.emplace_back(
            magix::compile::assembler_errors::ConfigRedefinition{
                dir_tok,
            }
        );
    }
    else
    {
        out = config_value;
    }
    return true;
}

auto
Assembler::parse_directive() -> bool
{
    // . has been checked
    ++current_token;
    auto [has, ident] = eat_token(magix::compile::TokenType::IDENTIFIER);
    if (!has)
    {
        return false;
    }

    auto command = ident.content;
    // PUT DATA COMMAND
    if (command == U"u8")
    {
        return parse_data_directive<magix::u8>();
    }
    else if (command == U"u16")
    {
        return parse_data_directive<magix::u16>();
    }
    else if (command == U"u32")
    {
        return parse_data_directive<magix::u32>();
    }
    else if (command == U"u64")
    {
        return parse_data_directive<magix::u64>();
    }
    else if (command == U"i8")
    {
        return parse_data_directive<magix::i8>();
    }
    else if (command == U"i16")
    {
        return parse_data_directive<magix::i16>();
    }
    else if (command == U"i32")
    {
        return parse_data_directive<magix::i32>();
    }
    else if (command == U"i64")
    {
        return parse_data_directive<magix::i64>();
    }
    else if (command == U"f32")
    {
        return parse_data_directive<magix::f32>();
    }
    else if (command == U"f64")
    {
        return parse_data_directive<magix::f64>();
    }
    // SIZE CONFIG COMMANDS
    else if (command == U"stack_size")
    {
        return parse_config_value(stack_size, ident);
    }
    else if (command == U"fork_size")
    {
        return parse_config_value(fork_size, ident);
    }
    else if (command == U"shared_size")
    {
        return parse_config_value(shared_size, ident);
    }
    else if (command == U"objcount")
    {
        return parse_config_value(obj_count, ident);
    }
    else if (command == U"fork_objcount")
    {
        return parse_config_value(obj_fork_count, ident);
    }
    else if (command == U"shared_objcount")
    {
        return parse_config_value(obj_shared_count, ident);
    }
    else
    {
        error_stack.emplace_back(
            magix::compile::assembler_errors::UnknownDirective{
                ident,
            }
        );
        return false;
    }
}

auto
Assembler::parse_entry() -> bool
{
    // @ has been checked
    ++current_token;
    return parse_label(true);
}

auto
Assembler::parse_statement() -> bool
{
    switch (current_token->type)
    {
    case magix::compile::TokenType::LINE_END:
    {
        return eat_token(magix::compile::TokenType::LINE_END).found;
    }
    case magix::compile::TokenType::IDENTIFIER:
    {
        return parse_label_or_instruction();
    }
    case magix::compile::TokenType::ENTRY_MARKER:
    {
        return parse_entry();
    }
    case magix::compile::TokenType::DIRECTIVE_MARKER:
    {
        return parse_directive();
    }
    default:
    {
        error_stack.emplace_back(
            magix::compile::assembler_errors::UnexpectedToken{
                *current_token,
                magix::compile::TokenType::LINE_END | magix::compile::TokenType::IDENTIFIER | magix::compile::TokenType::ENTRY_MARKER |
                    magix::compile::TokenType::DIRECTIVE_MARKER,
            }
        );
        return false;
    }
    }
}

void
Assembler::discard_remaining_line()
{
    for (; current_token->type != magix::compile::TokenType::LINE_END; ++current_token)
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
Assembler::reset_to_src(magix::span<const magix::compile::SrcToken> tokens)
{
    current_token = tokens.begin();
    end_token = tokens.end();

    unbound_labels.clear();
    entry_labels.clear();
    labels.clear();

    remap_cache.clear();

    error_stack.clear();

    linker_tasks.clear();

    data_segment.clear();
    code_segment.clear();
}

void
Assembler::link(magix::compile::ByteCodeRaw &out)
{
    // first copy ROM to out

    // we do datasegment, then code segment
    // code segment has alignment of code_word, ie u16
    // so align
    align_data_segment(magix::code_align_v<magix::code_word>);

    magix::span<const std::byte> data = data_segment;
    magix::span<const std::byte> code = magix::span(code_segment).as_const_bytes();

    size_t requested_size = data.size() + code.size();
    if (requested_size > magix::compile::byte_code_size)
    {
        error_stack.emplace_back(
            magix::compile::assembler_errors::CompilationTooBig{
                requested_size,
                magix::compile::byte_code_size,
            }
        );
        return;
    };

    // copy and get pointers
    std::byte *out_it_base = out.code;
    std::byte *out_it_data = out_it_base;
    std::byte *out_it_code = std::copy(data.begin(), data.end(), out_it_data);
    std::byte *out_it_end = std::copy(code.begin(), code.end(), out_it_code);
    std::fill(out_it_end, std::end(out.code), std::byte{});

    // the basic ROM is now set up
    // now we just need to fix all those linker tasks
    for (const auto &task : linker_tasks)
    {
        // find out the value to write
        magix::u16 value = 0;
        if (auto search = labels.find(task.label_token.content); search != labels.end())
        {
            LabelData label_data = search->second;
            switch (label_data.mode)
            {
            case LabelData::LabelMode::UNBOUND_NORMAL:
            case LabelData::LabelMode::UNBOUND_ENTRY:
            {
                error_stack.emplace_back(
                    magix::compile::assembler_errors::UnboundLabel{
                        task.label_token,
                    }
                );
                // next label
                continue;
            }
            case LabelData::LabelMode::DATA:
            {
                value = static_cast<magix::u16>(label_data.offset + out_it_data - out_it_base);
                break;
            }
            case LabelData::LabelMode::CODE:
            {
                value = static_cast<magix::u16>(label_data.offset + out_it_code - out_it_base);
                break;
            }
            case LabelData::LabelMode::ABSOLUTE:
            {
                value = label_data.offset;
                break;
            }
            }
        }
        else
        {
            error_stack.emplace_back(
                magix::compile::assembler_errors::UnboundLabel{
                    task.label_token,
                }
            );
            continue;
        }

        // find relocated address to write to
        std::byte *base = nullptr;
        switch (task.segment)
        {
        case LinkerTask::Segment::DATA:
        {
            base = out_it_data;
            break;
        }
        case LinkerTask::Segment::CODE:
            base = out_it_code;
            break;
        }
        std::byte *write_loc = base + task.offset;

        if (write_loc + 2 > out_it_end)
        {
            // should never happen!
            error_stack.emplace_back(
                magix::compile::assembler_errors::InternalError{
                    __LINE__,
                }
            );
            continue;
        }

        // note: even if the weird IBM-Mainframe-DX or whatever absurd platform does not provide the types we want, we always copy to the
        // front this way write_loc is okay and we don't have to do anything with padding
        switch (task.mode)
        {
        case LinkerTask::Mode::OVERWRITE:
        {
            // value as is
            break;
        }
        case LinkerTask::Mode::ADD:
            magix::u16 old_value;
            std::memcpy(&old_value, write_loc, sizeof(old_value));
            value = old_value + value;
            break;
        }
        std::memcpy(write_loc, &value, sizeof(value));
    }

    out.entry_points.clear();
    for (magix::compile::SrcView label_name : entry_labels)
    {
        godot::String persist_name;
        auto err = persist_name.resize(label_name.size() + 1);
        if (err)
        {
            // memallocation failed... ugh...
            // hope that never happens
            error_stack.emplace_back(
                magix::compile::assembler_errors::InternalError{
                    __LINE__,
                }
            );
            continue;
        }
        *std::copy(label_name.begin(), label_name.end(), persist_name.ptrw()) = 0;
        if (auto search = labels.find(label_name); search != labels.end())
        {
            LabelData data = search->second;
            switch (data.mode)
            {
            case LabelData::LabelMode::CODE:
            {
                out.entry_points[persist_name] = static_cast<magix::u16>(data.offset + out_it_code - out_it_base);
                break;
            }
            case LabelData::LabelMode::UNBOUND_ENTRY:
            case LabelData::LabelMode::UNBOUND_NORMAL:
            {
                error_stack.emplace_back(
                    magix::compile::assembler_errors::UnboundLabel{
                        data.declaration,
                    }
                );
                break;
            }
            case LabelData::LabelMode::DATA:
            case LabelData::LabelMode::ABSOLUTE:
            {
                // how?!? we checked that this never happens!
                error_stack.emplace_back(
                    magix::compile::assembler_errors::InternalError{
                        __LINE__,
                    }
                );
                continue;
            }
            }
        }
        else
        {
            // why is there an entry label without backing data???
            error_stack.emplace_back(
                magix::compile::assembler_errors::InternalError{
                    __LINE__,
                }
            );
            continue;
        }
    }

    // write configured values
    out.stack_size = stack_size.value_or(magix::execute::stack_size_default);
    out.fork_size = fork_size.value_or(0);
    out.shared_size = shared_size.value_or(0);
    out.obj_count = obj_count.value_or(0);
    out.obj_fork_count = obj_fork_count.value_or(0);
    out.obj_shared_count = obj_shared_count.value_or(0);
}

auto
magix::compile::assemble(magix::span<const magix::compile::SrcToken> tokens, magix::compile::ByteCodeRaw &out)
    -> std::vector<magix::compile::AssemblerError>
{
    Assembler assembler;
    // TODO: properly assert this
    // assert(tokens.back().type == magix::compile::TokenType::END_OF_FILE);

    // reset
    assembler.reset_to_src(tokens);

    // parse, this will call back into every emit function
    assembler.parse_program();

    if (assembler.error_stack.empty())
    {
        assembler.link(out);
    }

    return std::move(assembler.error_stack);
}

// ----- //
// TESTS //
// ----- //

#ifdef MAGIX_BUILD_TESTS

#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"

#include "godot_cpp/templates/pair.hpp"

#include <ostream>

namespace
{

auto
operator<<(std::ostream &ostream, const LabelData &label) -> std::ostream &
{
    switch (label.mode)
    {
    case LabelData::LabelMode::UNBOUND_ENTRY:
    {
        ostream << '@';
        break;
    }
    case LabelData::LabelMode::UNBOUND_NORMAL:
    {
        ostream << '?';
        break;
    }
    case LabelData::LabelMode::DATA:
    {
        ostream << 'd';
        break;
    }
    case LabelData::LabelMode::CODE:
    {
        ostream << 'c';
        break;
    }
    case LabelData::LabelMode::ABSOLUTE:
    {
        ostream << 'a';
        break;
    }
    }
    return ostream << '+' << label.offset << '@' << label.declaration;
}

auto
operator<<(std::ostream &ostream, const std::pair<const magix::compile::SrcView, LabelData> &labelmap) -> std::ostream &
{
    return magix::compile::print_srcsview(ostream, labelmap.first) << "->" << labelmap.second;
}

auto
operator<<(std::ostream &ostream, const LinkerTask &task) -> std::ostream &
{
    switch (task.mode)
    {
    case LinkerTask::Mode::OVERWRITE:
    {
        ostream << '=';
        break;
    }
    case LinkerTask::Mode::ADD:
    {
        ostream << '+';
        break;
    }
    }
    return ostream << task.label_token << '@';
    switch (task.segment)
    {

    case LinkerTask::Segment::DATA:
    {
        ostream << 'c';
        break;
    }
    case LinkerTask::Segment::CODE:
    {
        ostream << 'c';
        break;
    }
    }

    return ostream << task.offset;
}

auto
operator<<(std::ostream &ostream, const TrackRemapRegister &remap_reg) -> std::ostream &
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
        return magix::compile::print_srcsview(ostream << '[', remap_reg.label_declaration.content) << "]+" << remap_reg.offset;
    }
    }
    // TODO: unreachable
    return ostream;
}

auto
operator<<(std::ostream &ostream, const TrackRemapInstruction &remap_inst) -> std::ostream &
{
    if (remap_inst.user_generated)
    {
        ostream << "U:";
    }
    magix::compile::print_srcsview(ostream, remap_inst.root_instruction.content);
    for (const auto &reg : remap_inst.registers)
    {
        ostream << ' ' << reg;
    }
    return ostream;
}
} // namespace

#ifndef _CLANGD
// stuff large enough to weigh us down...
#include "assembler.test_literals.inc"
#endif

TEST_SUITE("assembler/prepare_pseudo")
{
    TEST_CASE("nop")
    {
        Assembler assm;
        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {0, 0},
                {0, 1},
                U"nop",
            },
            {
                magix::compile::TokenType::LINE_END,
                {0, 0},
                {1, 0},
                U"\n",
            },
        };
        assm.reset_to_src(tokens);
        auto res = assm.parse_prepare_pseudo_instruction();
        CHECK(res.parse_ok);
        CHECK(res.instruction_emitted);
        std::array<magix::compile::AssemblerError, 0> expect_error;
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

    TEST_CASE("__unittest.put.u32 $4")
    {
        Assembler assm;
        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {0, 0},
                {0, 1},
                U"__unittest.put.u32",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {0, 1},
                {0, 2},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {0, 2},
                {0, 3},
                U"4",
            },
            {
                magix::compile::TokenType::LINE_END,
                {0, 3},
                {1, 0},
                U"\n",
            },
        };
        assm.reset_to_src(tokens);
        auto res = assm.parse_prepare_pseudo_instruction();
        CHECK(res.parse_ok);
        CHECK(res.instruction_emitted);
        std::array<magix::compile::AssemblerError, 0> expect_error;
        CHECK_RANGE_EQ(assm.error_stack, expect_error);
        TrackRemapInstruction expect_inst[] = {{
            tokens[0],
            U"__unittest.put.u32",
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

    TEST_CASE("__unittest.put.u32 $4,")
    {
        Assembler assm;
        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {0, 0},
                {0, 1},
                U"__unittest.put.u32",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {0, 1},
                {0, 2},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {0, 2},
                {0, 3},
                U"4",
            },
            {
                magix::compile::TokenType::COMMA,
                {0, 3},
                {0, 4},
                U",",
            },
            {
                magix::compile::TokenType::LINE_END,
                {0, 4},
                {1, 0},
                U"\n",
            },
        };
        assm.reset_to_src(tokens);
        auto res = assm.parse_prepare_pseudo_instruction();
        CHECK(res.parse_ok);
        CHECK(res.instruction_emitted);
        std::array<magix::compile::AssemblerError, 0> expect_error;
        CHECK_RANGE_EQ(assm.error_stack, expect_error);
        TrackRemapInstruction expect_inst[] = {{
            tokens[0],
            U"__unittest.put.u32",
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

    TEST_CASE("__unittest.put.u32 $-4,")
    {
        Assembler assm;
        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {0, 0},
                {0, 1},
                U"__unittest.put.u32",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {0, 1},
                {0, 2},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {0, 2},
                {0, 3},
                U"-4",
            },
            {
                magix::compile::TokenType::COMMA,
                {0, 3},
                {0, 4},
                U",",
            },
            {
                magix::compile::TokenType::LINE_END,
                {0, 4},
                {1, 0},
                U"\n",
            },
        };
        assm.reset_to_src(tokens);
        auto res = assm.parse_prepare_pseudo_instruction();
        CHECK(res.parse_ok);
        CHECK(res.instruction_emitted);
        std::array<magix::compile::AssemblerError, 0> expect_error;
        CHECK_RANGE_EQ(assm.error_stack, expect_error);
        TrackRemapInstruction expect_inst[] = {{
            tokens[0],
            U"__unittest.put.u32",
            {{
                {
                    TrackRemapRegister::Type::LOCAL,
                    static_cast<magix::u16>(-4),
                    0,
                    tokens[2],
                },
            }},
            true,
        }};
        CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
    }

    TEST_CASE("set.i16 $4, #8")
    {
        Assembler assm;
        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {0, 0},
                {0, 1},
                U"set.i16",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {0, 1},
                {0, 2},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {0, 2},
                {0, 3},
                U"4",
            },
            {
                magix::compile::TokenType::COMMA,
                {0, 3},
                {0, 4},
                U",",
            },
            {
                magix::compile::TokenType::IMMEDIATE_MARKER,
                {0, 4},
                {0, 5},
                U"#",
            },
            {
                magix::compile::TokenType::NUMBER,
                {0, 5},
                {0, 6},
                U"8",
            },
            {
                magix::compile::TokenType::LINE_END,
                {0, 4},
                {1, 0},
                U"\n",
            },
        };
        assm.reset_to_src(tokens);
        auto res = assm.parse_prepare_pseudo_instruction();
        CHECK(res.parse_ok);
        CHECK(res.instruction_emitted);
        std::array<magix::compile::AssemblerError, 0> expect_error;
        CHECK_RANGE_EQ(assm.error_stack, expect_error);
        TrackRemapInstruction expect_inst[] = {{
            tokens[0],
            U"set.i16",
            {{{
                  TrackRemapRegister::Type::LOCAL,
                  4,
                  0,
                  tokens[2],
              },
              {
                  TrackRemapRegister::Type::IMMEDIATE_SET,
                  8,
                  0,
                  tokens[5],
              }}},
            true,
        }};
        CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
    }

    TEST_CASE("set.i16 $4, #test")
    {
        Assembler assm;
        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {0, 0},
                {0, 1},
                U"set.i16",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {0, 1},
                {0, 2},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {0, 2},
                {0, 3},
                U"4",
            },
            {
                magix::compile::TokenType::COMMA,
                {0, 3},
                {0, 4},
                U",",
            },
            {
                magix::compile::TokenType::IMMEDIATE_MARKER,
                {0, 4},
                {0, 5},
                U"#",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {0, 5},
                {0, 6},
                U"test",
            },
            {
                magix::compile::TokenType::LINE_END,
                {0, 4},
                {1, 0},
                U"\n",
            },
        };
        assm.reset_to_src(tokens);
        auto res = assm.parse_prepare_pseudo_instruction();
        CHECK(res.parse_ok);
        CHECK(res.instruction_emitted);
        std::array<magix::compile::AssemblerError, 0> expect_error;
        CHECK_RANGE_EQ(assm.error_stack, expect_error);
        TrackRemapInstruction expect_inst[] = {{
            tokens[0],
            U"set.i16",
            {{{
                  TrackRemapRegister::Type::LOCAL,
                  4,
                  0,
                  tokens[2],
              },
              {
                  TrackRemapRegister::Type::IMMEDIATE_TOKEN,
                  0,
                  0,
                  tokens[5],
              }}},
            true,
        }};
        CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
    }
}

TEST_SUITE("assembler/full")
{

    TEST_CASE("assembler: add.u32.imm $32, $24, #128")
    {
        Assembler assembler;

        const magix::compile::InstructionSpec *spec = magix::compile::get_instruction_spec(U"add.u32.imm");
        if (!CHECK_NE(spec, nullptr))
        {
            return;
        }

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"add.u32.imm",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {},
                {},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"32",
            },
            {
                magix::compile::TokenType::COMMA,
                {},
                {},
                U",",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {},
                {},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"28",
            },
            {
                magix::compile::TokenType::COMMA,
                {},
                {},
                U",",
            },
            {
                magix::compile::TokenType::IMMEDIATE_MARKER,
                {},
                {},
                U"#",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"128",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        // test up to link

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_pre_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_pre_link);

        CHECK(assembler.remap_cache.empty());

        magix::ranges::empty_range<const magix::compile::SrcView> expected_entries;
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        magix::ranges::empty_range<const std::pair<const magix::compile::SrcView, LabelData>> expected_labels;
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        const magix::code_word expected_code[] = {spec->opcode, 32, 28, 128};
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_const_bytes();
        magix::ranges::empty_range<const std::byte> expect_data;
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_post_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        magix::code_word expected_bytecode_u[8] = {spec->opcode, 32, 28, 128};
        auto expected_bc = magix::span(expected_bytecode_u).as_const_bytes();
        auto is_bytecode = magix::span(bc.code).as_const_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        magix::ranges::empty_range<const godot::KeyValue<godot::String, magix::u16>> entry_linked;
        ;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }

    TEST_CASE("assembler: add.u32.imm $32, $28, #label\\n@label:\\n nonop")
    {
        Assembler assembler;

        const magix::compile::InstructionSpec *spec_add_u32_imm = magix::compile::get_instruction_spec(U"add.u32.imm");
        if (!CHECK_NE(spec_add_u32_imm, nullptr))
        {
            return;
        }

        const magix::compile::InstructionSpec *spec_nonop = magix::compile::get_instruction_spec(U"nonop");
        if (!CHECK_NE(spec_nonop, nullptr))
        {
            return;
        }

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"add.u32.imm",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {},
                {},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"32",
            },
            {
                magix::compile::TokenType::COMMA,
                {},
                {},
                U",",
            },
            {
                magix::compile::TokenType::REGISTER_MARKER,
                {},
                {},
                U"$",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"28",
            },
            {
                magix::compile::TokenType::COMMA,
                {},
                {},
                U",",
            },
            {
                magix::compile::TokenType::IMMEDIATE_MARKER,
                {},
                {},
                U"#",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"label",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\n",
            },
            {
                magix::compile::TokenType::ENTRY_MARKER,
                {},
                {},
                U"@",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"label",
            },
            {
                magix::compile::TokenType::LABEL_MARKER,
                {},
                {},
                U":",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\n",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"nonop",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        // test up to link

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_pre_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_pre_link);

        CHECK(assembler.remap_cache.empty());

        const magix::compile::SrcView expected_entries[] = {
            U"label",
        };
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        std::pair<const magix::compile::SrcView, LabelData> expected_labels[] = {
            {
                U"label",
                {
                    LabelData::LabelMode::CODE,
                    8,
                    tokens[11],
                },
            },
        };
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        const magix::code_word expected_code[] = {spec_add_u32_imm->opcode, 32, 28, 0};
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_const_bytes();
        magix::ranges::empty_range<const std::byte> expect_data;
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        const LinkerTask expected_linker[] = {{
            LinkerTask::Mode::ADD,
            LinkerTask::Segment::CODE,
            6,
            tokens[8],
        }};
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_post_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        magix::code_word expected_bytecode_u[8] = {spec_add_u32_imm->opcode, 32, 28, 8};
        auto expected_bc = magix::span(expected_bytecode_u).as_const_bytes();
        auto is_bytecode = magix::span(bc.code).as_const_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        const godot::KeyValue<godot::String, magix::u16> entry_linked[] = {{"label", 8}};
        ;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }

    TEST_CASE("assembler: .u8 0x0F")
    {
        Assembler assembler;

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::DIRECTIVE_MARKER,
                {},
                {},
                U".",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"u8",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"0x0F",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        magix::ranges::empty_range<const magix::compile::AssemblerError> expected_errs;
        CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

        CHECK(assembler.remap_cache.empty());

        magix::ranges::empty_range<const magix::compile::SrcView> expected_entries;
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        magix::ranges::empty_range<const std::pair<const magix::compile::SrcView, LabelData>> expected_labels;
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        magix::ranges::empty_range<const magix::code_word> expected_code;
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_const_bytes();
        const magix::u8 expect_bytes_u[] = {
            0x0f,
        };
        auto expect_data = magix::span(expect_bytes_u).as_const_bytes();
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_post_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        magix::code_word expected_bytecode_u[8] = {0x0f}; // yeah kinda expect little endian
        auto expected_bc = magix::span(expected_bytecode_u).as_const_bytes();
        auto is_bytecode = magix::span(bc.code).as_const_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        magix::ranges::empty_range<const godot::KeyValue<godot::String, magix::u16>> entry_linked;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }

    TEST_CASE("assembler: .u8 -0x0F")
    {
        Assembler assembler;

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::DIRECTIVE_MARKER,
                {},
                {},
                U".",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"u8",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"-0x0F",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        const magix::compile::AssemblerError expected_errs[] = {
            magix::compile::assembler_errors::NumberNotRepresentable{
                tokens[2],
            },
        };
        CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

        CHECK(assembler.remap_cache.empty());

        magix::ranges::empty_range<const magix::compile::SrcView> expected_entries;
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        magix::ranges::empty_range<const std::pair<const magix::compile::SrcView, LabelData>> expected_labels;
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        magix::ranges::empty_range<const magix::code_word> expected_code;
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_const_bytes();
        magix::ranges::empty_range<const std::byte> expect_data;
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);
    }

    TEST_CASE("assembler: .i8 -0x0F")
    {
        Assembler assembler;

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::DIRECTIVE_MARKER,
                {},
                {},
                U".",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"i8",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"-0x0F",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        magix::ranges::empty_range<magix::compile::AssemblerError> expected_errs;
        CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

        CHECK(assembler.remap_cache.empty());

        magix::ranges::empty_range<const magix::compile::SrcView> expected_entries;
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        magix::ranges::empty_range<const std::pair<const magix::compile::SrcView, LabelData>> expected_labels;
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        magix::ranges::empty_range<const magix::code_word> expected_code;
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_const_bytes();
        const magix::u8 expect_bytes_u[] = {
            magix::u8(-0x0f),
        };
        auto expect_data = magix::span(expect_bytes_u).as_const_bytes();
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_post_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        magix::u8 expected_bytecode_u[16] = {magix::u8(-0x0f)}; // just this once
        auto expected_bc = magix::span(expected_bytecode_u).as_const_bytes();
        auto is_bytecode = magix::span(bc.code).as_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        magix::ranges::empty_range<const godot::KeyValue<godot::String, magix::u16>> entry_linked;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }

    TEST_CASE("assembler: .u16 0x1234")
    {
        Assembler assembler;

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::DIRECTIVE_MARKER,
                {},
                {},
                U".",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"u16",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"0x1234",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        magix::ranges::empty_range<magix::compile::AssemblerError> expected_errs;
        CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

        CHECK(assembler.remap_cache.empty());

        magix::ranges::empty_range<const magix::compile::SrcView> expected_entries;
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        magix::ranges::empty_range<const std::pair<const magix::compile::SrcView, LabelData>> expected_labels;
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        magix::ranges::empty_range<const magix::code_word> expected_code;
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_const_bytes();
        const magix::u8 expect_bytes_u[] = {
            0x34,
            0x12, // lil'endian
        };
        auto expect_data = magix::span(expect_bytes_u).as_const_bytes();
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_post_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        magix::code_word expected_bytecode_u[8] = {0x1234};
        auto expected_bc = magix::span(expected_bytecode_u).as_const_bytes();
        auto is_bytecode = magix::span(bc.code).as_const_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        magix::ranges::empty_range<const godot::KeyValue<godot::String, magix::u16>> entry_linked;
        ;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }

    TEST_CASE("assembler: .u32 0x12345678")
    {
        Assembler assembler;

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::DIRECTIVE_MARKER,
                {},
                {},
                U".",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"u32",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"0x12345678",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        magix::ranges::empty_range<magix::compile::AssemblerError> expected_errs;
        CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

        CHECK(assembler.remap_cache.empty());

        magix::ranges::empty_range<const magix::compile::SrcView> expected_entries;
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        magix::ranges::empty_range<const std::pair<const magix::compile::SrcView, LabelData>> expected_labels;
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        magix::ranges::empty_range<const magix::code_word> expected_code;
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_bytes();
        const magix::u8 expect_bytes_u[] = {
            0x78, 0x56, 0x34,
            0x12, // lil'endian
        };
        auto expect_data = magix::span(expect_bytes_u).as_bytes();
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_post_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        magix::u32 expected_bytecode_u[4] = {0x12345678}; // yeah, I don't care about writing it down word for word
        auto expected_bc = magix::span(expected_bytecode_u).as_bytes();
        auto is_bytecode = magix::span(bc.code).as_const_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        magix::ranges::empty_range<const godot::KeyValue<godot::String, magix::u16>> entry_linked;
        ;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }

    TEST_CASE("assembler: .u64 0x123456789abcdef0")
    {
        Assembler assembler;

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::DIRECTIVE_MARKER,
                {},
                {},
                U".",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"u64",
            },
            {
                magix::compile::TokenType::NUMBER,
                {},
                {},
                U"0x123456789abcdef0",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        magix::ranges::empty_range<magix::compile::AssemblerError> expected_errs;
        CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

        CHECK(assembler.remap_cache.empty());

        magix::ranges::empty_range<const magix::compile::SrcView> expected_entries;
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        magix::ranges::empty_range<const std::pair<const magix::compile::SrcView, LabelData>> expected_labels;
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        magix::ranges::empty_range<const magix::compile::SrcToken> expected_unbound;
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        magix::ranges::empty_range<const magix::code_word> expected_code;
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_bytes();
        const magix::u8 expect_bytes_u[] = {
            0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, // lil'endian
        };
        auto expect_data = magix::span(expect_bytes_u).as_bytes();
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        magix::ranges::empty_range<const magix::compile::AssemblerError> errs_post_link;
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        magix::u64 expected_bytecode_u[2] = {0x123456789abcdef0}; // yeah, I don't care about writing it down word for word
        auto expected_bc = magix::span(expected_bytecode_u).as_bytes();
        auto is_bytecode = magix::span(bc.code).as_const_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        magix::ranges::empty_range<const godot::KeyValue<godot::String, magix::u16>> entry_linked;
        ;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }

    TEST_CASE("assembler: @entry:\\n <unbound>")
    {
        Assembler assembler;

        magix::compile::SrcToken tokens[] = {
            {
                magix::compile::TokenType::ENTRY_MARKER,
                {},
                {},
                U"@",
            },
            {
                magix::compile::TokenType::IDENTIFIER,
                {},
                {},
                U"entry",
            },
            {
                magix::compile::TokenType::LABEL_MARKER,
                {},
                {},
                U":",
            },
            {
                magix::compile::TokenType::LINE_END,
                {},
                {},
                U"\0",
            }
        };

        assembler.reset_to_src(tokens);
        assembler.parse_program();

        magix::ranges::empty_range<const magix::compile::AssemblerError> expected_errs;
        CHECK_RANGE_EQ(assembler.error_stack, expected_errs);

        CHECK(assembler.remap_cache.empty());

        const magix::compile::SrcView expected_entries[] = {
            U"entry",
        };
        CHECK_RANGE_EQ(assembler.entry_labels, expected_entries);

        const std::pair<const magix::compile::SrcView, LabelData> expected_labels[] = {
            {
                U"entry",
                {
                    LabelData::LabelMode::UNBOUND_ENTRY,
                    0,
                    tokens[1],
                },
            },
        };
        CHECK_RANGE_EQ(assembler.labels, expected_labels);

        const magix::compile::SrcToken expected_unbound[] = {
            tokens[1],
        };
        CHECK_RANGE_EQ(assembler.unbound_labels, expected_unbound);

        magix::ranges::empty_range<const magix::code_word> expected_code;
        CHECK_RANGE_EQ(assembler.code_segment, expected_code);

        auto is_data_seg = magix::span(assembler.data_segment).as_bytes();
        magix::ranges::empty_range<const magix::u8> expect_bytes_u;
        auto expect_data = magix::span(expect_bytes_u).as_bytes();
        CHECK_BYTESTRING_EQ(is_data_seg, expect_data);

        magix::ranges::empty_range<const LinkerTask> expected_linker;
        CHECK_RANGE_EQ(assembler.linker_tasks, expected_linker);

        if (!assembler.error_stack.empty())
        {
            return;
        }

        // now link and test

        magix::compile::ByteCodeRaw bc;
        assembler.link(bc);

        const magix::compile::AssemblerError errs_post_link[] = {magix::compile::assembler_errors::UnboundLabel{
            tokens[1],
        }};
        CHECK_RANGE_EQ(assembler.error_stack, errs_post_link);

        const magix::u8 expected_bytecode_u[16] = {};
        auto expected_bc = magix::span(expected_bytecode_u).as_bytes();
        auto is_bytecode = magix::span(bc.code).as_const_bytes().first<16>();
        CHECK_BYTESTRING_EQ(is_bytecode, expected_bc);

        magix::ranges::empty_range<const godot::KeyValue<godot::String, magix::u16>> entry_linked;
        ;
        CHECK_RANGE_EQ(bc.entry_points, entry_linked);
    }
}

#endif
