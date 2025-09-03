#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/instruction_data.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/doctest_helper.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"

#include <array>
#include <charconv>
#include <system_error>
#include <type_traits>
#include <vector>

#ifdef MAGIX_BUILD_TESTS
#include "magix_vm/compilation/printing.hpp"
#endif

namespace
{

struct LabelData
{
    magix::SrcToken declaration;
};

/** Tracks data to remap a signle register. That is the original token, added offset, etc. */
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
    uint16_t value;
    uint16_t offset;
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
    magix::SrcToken instruction_name_decl;
    std::array<TrackRemapRegister, magix::MAX_REGISTERS_PER_INSTRUCTION> registers;
    bool user_generated = false;

    constexpr bool
    operator==(const TrackRemapInstruction &rhs) const
    {
        return instruction_name_decl == rhs.instruction_name_decl && registers == rhs.registers && user_generated == rhs.user_generated;
    }

    constexpr bool
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
    case TrackRemapRegister::Type::IMMEDIATE_TOKEN:
        return magix::print_srcsview(ostream << '#', remap_reg.label_declaration.content) << '+' << remap_reg.offset;

    default:
    {
        return ostream << "INVALID";
    }
    }
}

std::ostream &
operator<<(std::ostream &ostream, const TrackRemapInstruction &remap_inst)
{
    if (remap_inst.user_generated)
    {
        ostream << "U:";
    }
    magix::print_srcsview(ostream, remap_inst.instruction_name_decl.content);
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
    bool arguments_truncated;
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
    bool
    eat_endline();

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
    parse_label();
    bool
    parse_instruction();
    ParsePreparePseudoResult
    parse_prepare_pseudo_instruction();
    void
    remap_emit_instruction();
    void
    emit_instruction(const TrackRemapInstruction &inst, const magix::InstructionSpec &spec);

    span_type::iterator_type current_token;
    span_type::iterator_type end_token;
    std::vector<LabelData> unresolved_tokens;
    std::vector<TrackRemapInstruction> remap_cache;

    std::vector<magix::AssemblerError> error_stack;
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
        error_stack.push_back({
            magix::AssemblerError::Type::NUMBER_INVALID,
            token,
        });
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
            error_stack.push_back({
                magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE,
                token,
            });
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
        error_stack.push_back({
            magix::AssemblerError::Type::NUMBER_INVALID,
            token,
        });
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
            error_stack.push_back({
                magix::AssemblerError::Type::NUMBER_INVALID,
                token,
            });
            return false;
        }
        else
        {
            char_stack.push_back(in);
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
                error_stack.push_back({
                    magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE,
                    token,
                });
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
        error_stack.push_back({
            magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE,
            token,
        });
        return false;
    }
    else if (result.ec != std::errc{} || result.ptr != num_end)
    {
        // other types of error
        // ptr is compared, becuase std::from_chars considers partially eating the number a success.
        error_stack.push_back({
            magix::AssemblerError::Type::NUMBER_INVALID,
            token,
        });
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
#define MAGIXTEST_PARSE_ERR(type, value, err)                                                                                              \
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
            {                                                                                                                              \
                err,                                                                                                                       \
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
    MAGIXTEST_PARSE_ERR(magix::u32, -1, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
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
    MAGIXTEST_PARSE_ERR(magix::u32, -0x1, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, -0x1);
}
TEST_CASE("assembler:parse float")
{
    MAGIXTEST_PARSE_ERR(magix::i32, 0.5, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_ERR(magix::u32, 0.5, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 0.5);
    MAGIXTEST_PARSE_ERR(magix::i32, -0.5, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_ERR(magix::u32, -0.5, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, -0.5);
}
TEST_CASE("assembler:parse float-hex")
{
    MAGIXTEST_PARSE_ERR(magix::i32, 0x1p-1, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_ERR(magix::u32, 0x1p-1, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, 0x1p-1);
    MAGIXTEST_PARSE_ERR(magix::i32, -0x1p-1, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_ERR(magix::u32, -0x1p-1, magix::AssemblerError::Type::NUMBER_VALUE_NOT_REPRESENTABLE);
    MAGIXTEST_PARSE_SIMPLE(magix::f32, -0x1p-1);
}

#endif

bool
Assembler::eat_endline()
{
    if (current_token->type == magix::TokenType::LINE_END)
    {
        ++current_token;
        return true;
    }
    else
    {
        error_stack.push_back({
            magix::AssemblerError::Type::UNEXPECTED_TOKEN,
            *current_token,
        });
        return false;
    }
}

bool
Assembler::parse_label()
{
    magix::SrcToken declaration = *current_token;
    unresolved_tokens.push_back({declaration});
    // decl and colon
    current_token += 2;
    return eat_endline();
}

ParsePreparePseudoResult
Assembler::parse_prepare_pseudo_instruction()
{
    // this prepares the pseudo instruction as written
    // say const.f32 #4 will be stored as "const.f32" with immediate "#4"
    // meaning is not checked yet

    // make space
    remap_cache.resize(1);
    TrackRemapInstruction &initial = remap_cache[0];
    initial = {}; // reset necessary!
    initial.user_generated = true;

    initial.instruction_name_decl = *current_token++;

    ParsePreparePseudoResult result{
        true,
        false,
    };

    size_t reg_count = 0;
    while (true)
    {
        switch (current_token->type)
        {
        case magix::TokenType::LINE_END:
        {
            // no more arguments
            ++current_token;
            return result;
        }
        case magix::TokenType::IMMEDIATE_MARKER:
        {
            ++current_token;

            if (current_token->type == magix::TokenType::NUMBER || current_token->type == magix::TokenType::IDENTIFIER)
            {
                if (reg_count >= magix::MAX_REGISTERS_PER_INSTRUCTION)
                {
                    result.arguments_truncated = true;
                }
                else
                {
                    initial.registers[reg_count++] = {
                        TrackRemapRegister::Type::IMMEDIATE_TOKEN,
                        0,
                        0,
                        *current_token,
                    };
                }
                ++current_token;
            }
            else
            {
                error_stack.push_back({
                    magix::AssemblerError::Type::UNEXPECTED_TOKEN, *current_token,
                    // TODO: this should report what is expected
                });
                result.parse_ok = false;
                return result;
            }

            if (current_token->type == magix::TokenType::COMMA)
            {
                ++current_token;
                continue;
            }
            else if (current_token->type == magix::TokenType::LINE_END)
            {
                ++current_token;
                return result;
            }
            else
            {
                error_stack.push_back({
                    magix::AssemblerError::Type::UNEXPECTED_TOKEN, *current_token,
                    // TODO: this should report what is expected
                });
                result.parse_ok = false;
                return result;
            }
        }
        case magix::TokenType::REGISTER_MARKER:
        {
            ++current_token;
            if (current_token->type == magix::TokenType::NUMBER)
            {
                if (reg_count >= magix::MAX_REGISTERS_PER_INSTRUCTION)
                {
                    result.arguments_truncated = true;
                }
                else
                {
                    magix::i16 reg_off;
                    if (extract_number(*current_token, reg_off))
                    {
                        initial.registers[reg_count++] = {
                            TrackRemapRegister::Type::LOCAL,
                            magix::u16(reg_off),
                            0,
                            *current_token,
                        };
                    }
                }
                ++current_token;
            }
            else
            {
                error_stack.push_back({
                    magix::AssemblerError::Type::UNEXPECTED_TOKEN, *current_token,
                    // TODO: this should report what is expected
                });
                result.parse_ok = false;
                return result;
            }

            if (current_token->type == magix::TokenType::COMMA)
            {
                ++current_token;
                continue;
            }
            else if (current_token->type == magix::TokenType::LINE_END)
            {
                ++current_token;
                return result;
            }
            else
            {
                error_stack.push_back({
                    magix::AssemblerError::Type::UNEXPECTED_TOKEN, *current_token,
                    // TODO: this should report what is expected
                });
                result.parse_ok = false;
                return result;
            }
        }
        default:
        {
            error_stack.push_back({
                magix::AssemblerError::Type::UNEXPECTED_TOKEN, *current_token,
                // TODO: this should report what is expected
            });
            result.parse_ok = false;
            return result;
        }
        }
    }
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
            U"instname",
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
    CHECK_EQ(res.parse_ok, true);
    CHECK_EQ(res.arguments_truncated, false);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {
        {
            tokens[0],
            {{}},
            true,
        },
    };
    CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
}

TEST_CASE("assembler/preppseudo $1,")
{
    Assembler assm;
    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {0, 0},
            {0, 1},
            U"instname",
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
    CHECK_EQ(res.parse_ok, true);
    CHECK_EQ(res.arguments_truncated, false);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
        {{
            {
                TrackRemapRegister::Type::LOCAL,
                1,
                0,
                tokens[2],
            },
        }},
        true,
    }};
    CHECK_RANGE_EQ(assm.remap_cache, expect_inst);
}

TEST_CASE("assembler/preppseudo $-1")
{
    Assembler assm;
    magix::SrcToken tokens[] = {
        {
            magix::TokenType::IDENTIFIER,
            {0, 0},
            {0, 1},
            U"instname",
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
    CHECK_EQ(res.parse_ok, true);
    CHECK_EQ(res.arguments_truncated, false);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
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
            U"instname",
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
    CHECK_EQ(res.parse_ok, true);
    CHECK_EQ(res.arguments_truncated, false);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
        {{
            {
                TrackRemapRegister::Type::IMMEDIATE_TOKEN,
                0,
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
            U"instname",
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
    CHECK_EQ(res.parse_ok, true);
    CHECK_EQ(res.arguments_truncated, false);
    std::array<magix::AssemblerError, 0> expect_error;
    CHECK_RANGE_EQ(assm.error_stack, expect_error);
    TrackRemapInstruction expect_inst[] = {{
        tokens[0],
        {{
            {
                TrackRemapRegister::Type::IMMEDIATE_TOKEN,
                0,
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
    // TODO
}

void
Assembler::remap_emit_instruction()
{
    // remap_cache is in reverse order, otherwise push/pop would be at the front
    while (!remap_cache.empty())
    {
        TrackRemapInstruction current = remap_cache.back();
        remap_cache.pop_back();

        const magix::InstructionSpec *spec = magix::get_instruction_spec(current.instruction_name_decl.content);

        if (!spec)
        {
            error_stack.push_back({
                magix::AssemblerError::Type::UNKNOWN_INSTRUCTION,
                current.instruction_name_decl,
            });
            continue;
        }

        if (!spec->is_pseudo)
        {
            emit_instruction(current, *spec);
            continue;
        }

        // reverse order because otherwise we would need to push/pop front
        for (const magix::PseudoInstructionTranslation &translation : spec->pseudo_translations | magix::ranges::reverse_view)
        {
            TrackRemapInstruction emit{};
            emit.instruction_name_decl = magix::SrcToken{
                magix::TokenType::IDENTIFIER,
                magix::SrcLoc::max(),
                magix::SrcLoc::max(),
                translation.out_mnenomic,
            };
            for (auto ind : magix::ranges::num_range(magix::MAX_REGISTERS_PER_INSTRUCTION))
            {
                switch (translation.remaps[ind].type)
                {

                case magix::InstructionRegisterRemap::RemapType::UNUSED:
                {
                    // default is unused
                    // emit.registers[ind] = {};
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
    if (prepare.parse_ok && !prepare.arguments_truncated)
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
        return parse_label();
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
    // TODO: implement
    return false;
}

bool
Assembler::parse_statement()
{

    switch (current_token->type)
    {
    case magix::TokenType::LINE_END:
    {
        return eat_endline();
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
        error_stack.push_back({
            magix::AssemblerError::Type::UNEXPECTED_TOKEN,
            *current_token,
        });
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
    unresolved_tokens.clear();
    error_stack.clear();
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
}

void
assemble(magix::span<const magix::SrcToken> tokens)
{
    Assembler assembler;
    assembler.assemble(tokens);
}
