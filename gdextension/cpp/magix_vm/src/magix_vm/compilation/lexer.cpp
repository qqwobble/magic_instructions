#include "magix_vm/compilation/lexer.hpp"

#include <godot_cpp/variant/char_utils.hpp>

#include <algorithm>
#include <vector>

#ifdef MAGIX_BUILD_TESTS
#include "magix_vm/compilation/printing.hpp"
#include "magix_vm/doctest_helper.hpp"
#include <doctest.h>
#endif

namespace
{

[[nodiscard]] constexpr auto
is_number_start(magix::compile::SrcChar chr) -> bool
{
    if (chr >= '0' && chr <= '9')
    {
        return true;
    }
    if (chr == '+' || chr == '-')
    {
        return true;
    }
    return false;
}

[[nodiscard]] constexpr auto
is_number_continue(magix::compile::SrcChar chr) -> bool
{
    if (chr == '.' || chr == '+' || chr == '-')
    {
        // might appear somewhere in floats
        return true;
    }
    return godot::is_unicode_identifier_continue(chr);
}

[[nodiscard]] constexpr auto
is_ident_start(magix::compile::SrcChar chr) -> bool
{
    return godot::is_unicode_identifier_start(chr);
}

[[nodiscard]] constexpr auto
is_ident_continue(magix::compile::SrcChar chr) -> bool
{
    // also want those dots.
    return godot::is_unicode_identifier_continue(chr) || chr == '.';
}

struct Lexer
{
    using iterator = magix::compile::SrcView::const_iterator;
    iterator it;
    iterator end;

    magix::compile::SrcLoc current_loc = magix::compile::SrcLoc::zero();

    Lexer(magix::compile::SrcView src) : it(src.cbegin()), end(src.cend()) {}

    /** Make eof at current position, does not advance. */
    [[nodiscard]] auto
    make_eof() -> magix::compile::SrcToken
    {
        // the source view does not contain the \0 we need
        // so this returns a new view
        return {
            magix::compile::TokenType::LINE_END,
            current_loc,
            current_loc.newline(),
            U"\0",
        };
    }

    /** Consume one char, emit as token. */
    [[nodiscard]] auto
    read_atomic_token(magix::compile::TokenType type) -> magix::compile::SrcToken
    {
        magix::compile::SrcView view = {it++, 1};
        return {
            type,
            current_loc,
            current_loc.advance_column(),
            view,
        };
    }

    /** Consumes all identifier chars starting here. */
    [[nodiscard]] auto
    read_identifier_token() -> magix::compile::SrcToken
    {
        magix::compile::SrcLoc begin_loc = current_loc;
        iterator ident_begin = it;

        while (it != end)
        {
            if (is_ident_continue(*it))
            {
                ++it;
                current_loc.advance_column();
            }
            else
            {
                break;
            }
        }

        magix::compile::SrcLoc end_loc = current_loc;
        magix::compile::SrcView view{ident_begin, (size_t)(it - ident_begin)};

        return {
            magix::compile::TokenType::IDENTIFIER,
            begin_loc,
            end_loc,
            view,
        };
    }

    /** Consumes a string, starting with the quotes. Quotation marks are included. */
    [[nodiscard]] auto
    read_string_token() -> magix::compile::SrcToken
    {
        iterator begin_it = it;
        magix::compile::SrcLoc begin_loc = current_loc;

        // skip opening ", we already have seen it
        ++it;
        current_loc.advance_column();

        bool is_escaped = false;
        while (it != end)
        {
            magix::compile::SrcChar read = *it++;
            if (read == magix::compile::SYMBOL_NEWLINE)
            {
                current_loc.advance_newline();
            }
            else
            {
                current_loc.advance_column();
            }

            if (is_escaped)
            {
                is_escaped = false;
            }
            else
            {
                if (read == magix::compile::SYMBOL_ESCAPE)
                {
                    is_escaped = true;
                }
                else if (read == magix::compile::SYMBOL_QUOTES)
                {
                    magix::compile::SrcView view{begin_it, (size_t)(it - begin_it)};
                    return {
                        magix::compile::TokenType::STRING,
                        begin_loc,
                        current_loc,
                        view,
                    };
                }
            }
        }
        // not terminated
        magix::compile::SrcView view{begin_it, (size_t)(it - begin_it)};
        return {
            magix::compile::TokenType::UNTERMINATED_STRING,
            begin_loc,
            current_loc,
            view,
        };
    }

    [[nodiscard]] auto
    read_number_token() -> magix::compile::SrcToken
    {
        magix::compile::SrcLoc begin_loc = current_loc;
        iterator ident_begin = it;

        while (it != end)
        {
            if (is_number_continue(*it))
            {
                ++it;
                current_loc.advance_column();
            }
            else
            {
                break;
            }
        }

        magix::compile::SrcLoc end_loc = current_loc;
        magix::compile::SrcView view{ident_begin, (size_t)(it - ident_begin)};

        return {
            magix::compile::TokenType::NUMBER,
            begin_loc,
            end_loc,
            view,
        };
    }

    void
    skip_comment()
    {
        auto old_it = it;
        it = std::find(it, end, magix::compile::SYMBOL_NEWLINE);
        // does not eat the newline yet, as that is it's own token!
        current_loc.advance_column(it - old_it);
    }

    [[nodiscard]] auto
    next_token() -> magix::compile::SrcToken
    {
    restart:
        if (it == end)
        {
            return make_eof();
        }

        magix::compile::SrcChar first = *it;

        if (first == magix::compile::SYMBOL_NEWLINE)
        {
            magix::compile::SrcView view = {it++, 1};
            return {
                magix::compile::TokenType::LINE_END,
                current_loc,
                current_loc.advance_newline(),
                view,
            };
        }
        if (godot::is_whitespace(first))
        {
            ++it;
            current_loc.advance_column();
            goto restart; // fight me
        }
        if (first == magix::compile::SYMBOL_COMMENT)
        {
            skip_comment();
            goto restart;
        }
        if (first == magix::compile::SYMBOL_LABEL)
        {
            return read_atomic_token(magix::compile::TokenType::LABEL_MARKER);
        }
        if (first == magix::compile::SYMBOL_ENTRY)
        {
            return read_atomic_token(magix::compile::TokenType::ENTRY_MARKER);
        }
        if (first == magix::compile::SYMBOL_IMMEDIATE)
        {
            return read_atomic_token(magix::compile::TokenType::IMMEDIATE_MARKER);
        }
        if (first == magix::compile::SYMBOL_DIRECTIVE)
        {
            return read_atomic_token(magix::compile::TokenType::DIRECTIVE_MARKER);
        }
        if (first == magix::compile::SYMBOL_REGISTER)
        {
            return read_atomic_token(magix::compile::TokenType::REGISTER_MARKER);
        }
        if (first == magix::compile::SYMBOL_COMMA)
        {
            return read_atomic_token(magix::compile::TokenType::COMMA);
        }
        if (first == magix::compile::SYMBOL_QUOTES)
        {
            return read_string_token();
        }
        if (is_ident_start(first))
        {
            return read_identifier_token();
        }
        if (is_number_start(first))
        {
            return read_number_token();
        }

        // this char matches nothing!
        magix::compile::SrcView view{it++, 1};
        return {
            magix::compile::TokenType::INVALID_CHAR,
            current_loc,
            current_loc.advance_column(),
            view,
        };
    }
};

} // namespace

auto
magix::compile::lex(magix::compile::SrcView source) -> std::vector<magix::compile::SrcToken>
{
    Lexer lexer(source);
    std::vector<magix::compile::SrcToken> out;

    while (true)
    {
        SrcToken token = lexer.next_token();
        out.push_back(token);
        if (token.type == TokenType::LINE_END && token.content == U"\0")
        {
            break;
        }
    }

    return out;
}

#ifdef MAGIX_BUILD_TESTS

TEST_SUITE("lexer")
{
    auto eof_line_end = [](magix::compile::SrcView src) {
        return magix::compile::SrcToken{
            magix::compile::TokenType::LINE_END,
            {0, src.length()},
            {1, 0},
            U"\0",
        };
    };
    auto eof_at = [](magix::compile::SrcLoc loc) {
        return magix::compile::SrcToken{
            magix::compile::TokenType::LINE_END,
            loc,
            loc.newline(),
            U"\0",
        };
    };
    TEST_CASE("no-token")
    {

        SUBCASE("empty")
        {
            magix::compile::SrcView src = U"";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {eof_at({0, 0})};
            CHECK_RANGE_EQ(got, expected);
        }

        SUBCASE(";")
        {
            magix::compile::SrcView src = U";";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {eof_at({0, 1})};
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("; foo")
        {
            magix::compile::SrcView src = U"; foo";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {eof_at({0, 5})};
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("newlines")
    {
        SUBCASE("\\n")
        {
            magix::compile::SrcView src = U"\n";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::LINE_END,
                    {0, 0},
                    {1, 0},
                    U"\n",
                },
                eof_at({1, 0}),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("\\n   ;   \\n  ;")
        {
            magix::compile::SrcView src = U"\n   ;   \n  ;";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::LINE_END,
                    {0, 0},
                    {1, 0},
                    U"\n",
                },
                {
                    magix::compile::TokenType::LINE_END,
                    {1, 7},
                    {2, 0},
                    U"\n",
                },
                eof_at({2, 3}),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE(";\\n;\\n;\\n;\\n;\\n;")
        {
            magix::compile::SrcView src = U";\n;\n;\n;\n;\n;";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::LINE_END,
                    {0, 1},
                    {1, 0},
                    U"\n",
                },
                {
                    magix::compile::TokenType::LINE_END,
                    {1, 1},
                    {2, 0},
                    U"\n",
                },
                {
                    magix::compile::TokenType::LINE_END,
                    {2, 1},
                    {3, 0},
                    U"\n",
                },
                {
                    magix::compile::TokenType::LINE_END,
                    {3, 1},
                    {4, 0},
                    U"\n",
                },
                {
                    magix::compile::TokenType::LINE_END,
                    {4, 1},
                    {5, 0},
                    U"\n",
                },
                eof_at({5, 1}),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("identifier")
    {

        SUBCASE("hello")
        {
            magix::compile::SrcView src = U"hello";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 5},
                    U"hello",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("foo bar baz")
        {
            magix::compile::SrcView src = U"foo bar baz";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 3},
                    U"foo",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 4},
                    {0, 7},
                    U"bar",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 8},
                    {0, 11},
                    U"baz",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("foo bar ; baz")
        {
            magix::compile::SrcView src = U"foo bar ; baz";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 3},
                    U"foo",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 4},
                    {0, 7},
                    U"bar",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("ß α ඞ")
        {
            // i know I am very funny
            magix::compile::SrcView src = U"ß α ඞ";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 1},
                    U"ß",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 3},
                    U"α",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 4},
                    {0, 5},
                    U"ඞ",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("String")
    {
        SUBCASE("\"hello world!\"")
        {
            magix::compile::SrcView src = U"\"hello world!\"";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::STRING,
                    {0, 0},
                    {0, 14},
                    U"\"hello world!\"",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("\"hello\" \"world!\"")
        {
            magix::compile::SrcView src = U"\"hello\" \"world!\"";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::STRING,
                    {0, 0},
                    {0, 7},
                    U"\"hello\"",
                },
                {
                    magix::compile::TokenType::STRING,
                    {0, 8},
                    {0, 16},
                    U"\"world!\"",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("\"hello ")
        {
            magix::compile::SrcView src = U"\"hello ";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::UNTERMINATED_STRING,
                    {0, 0},
                    {0, 7},
                    U"\"hello ",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("Number")
    {
        SUBCASE("single digit")
        {
            magix::compile::SrcChar buffer[1];
            magix::compile::SrcView src(buffer, 1);
            for (int dig = 0; dig < 10; ++dig)
            {
                CAPTURE(dig);
                buffer[0] = U'0' + dig;
                std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
                std::vector<magix::compile::SrcToken> expected = {
                    {
                        magix::compile::TokenType::NUMBER,
                        {0, 0},
                        {0, 1},
                        src,
                    },
                    eof_line_end(src),
                };
                CHECK_RANGE_EQ(got, expected);
            }
        }
        SUBCASE("double digit")
        {
            magix::compile::SrcChar buffer[2];
            magix::compile::SrcView src(buffer, 2);
            for (int number = 0; number < 100; ++number)
            {
                CAPTURE(number);
                buffer[0] = U'0' + number / 10;
                buffer[1] = U'0' + number % 10;
                std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
                std::vector<magix::compile::SrcToken> expected = {
                    {
                        magix::compile::TokenType::NUMBER,
                        {0, 0},
                        {0, 2},
                        src,
                    },
                    eof_line_end(src),
                };
                CHECK_RANGE_EQ(got, expected);
            }
        }
        SUBCASE("double digit with _")
        {
            magix::compile::SrcChar buffer[3];
            buffer[1] = U'_';
            magix::compile::SrcView src(buffer, 3);
            for (int number = 0; number < 100; ++number)
            {
                CAPTURE(number);
                buffer[0] = U'0' + number / 10;
                buffer[2] = U'0' + number % 10;
                std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
                std::vector<magix::compile::SrcToken> expected = {
                    {
                        magix::compile::TokenType::NUMBER,
                        {0, 0},
                        {0, 3},
                        src,
                    },
                    eof_line_end(src),
                };
                CHECK_RANGE_EQ(got, expected);
            }
        }

        auto dig2hex = [](int d) -> magix::compile::SrcChar {
            if (d < 10)
            {
                return U'0' + d;
            }
            else
            {
                return 'A' + d - 10;
            }
        };
        SUBCASE("Hex double")
        {
            magix::compile::SrcChar buffer[4];
            buffer[0] = U'0';
            buffer[1] = U'x';
            magix::compile::SrcView src(buffer, 4);
            for (int number = 0; number < 0x100; ++number)
            {
                CAPTURE(number);
                buffer[2] = dig2hex(number / 16);
                buffer[3] = dig2hex(number % 16);
                std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
                std::vector<magix::compile::SrcToken> expected = {
                    {
                        magix::compile::TokenType::NUMBER,
                        {0, 0},
                        {0, 4},
                        src,
                    },
                    eof_line_end(src),
                };
                CHECK_RANGE_EQ(got, expected);
            }
        }
        SUBCASE("Float 1.0")
        {
            magix::compile::SrcView src = U"1.0";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 0},
                    {0, 3},
                    U"1.0",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("Pi")
        {
            magix::compile::SrcView src = U"3.1415926535897932384626433";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 0},
                    {0, 27},
                    U"3.1415926535897932384626433",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("Prepend +1.0")
        {
            magix::compile::SrcView src = U"+1.0";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 0},
                    {0, 4},
                    U"+1.0",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("Prepend -1.0")
        {
            magix::compile::SrcView src = U"-1.0";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 0},
                    {0, 4},
                    U"-1.0",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("Invalid continue")
        {
            // to avoid this whole is it hex or following variable mess
            // just parse everything as invalid number
            // parser, or whoever uses it downstream must validate it
            magix::compile::SrcView src = U"-1.0abc";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 0},
                    {0, 7},
                    U"-1.0abc",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("Entry")
    {
        SUBCASE("@")
        {
            magix::compile::SrcView src = U"@";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::ENTRY_MARKER,
                    {0, 0},
                    {0, 1},
                    src,
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("@ label :")
        {
            magix::compile::SrcView src = U"@ label :";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::ENTRY_MARKER,
                    {0, 0},
                    {0, 1},
                    U"@",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 7},
                    U"label",
                },
                {
                    magix::compile::TokenType::LABEL_MARKER,
                    {0, 8},
                    {0, 9},
                    U":",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("@label:")
        {
            magix::compile::SrcView src = U"@label:";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::ENTRY_MARKER,
                    {0, 0},
                    {0, 1},
                    U"@",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::compile::TokenType::LABEL_MARKER,
                    {0, 6},
                    {0, 7},
                    U":",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("IMMEDIATE")
    {
        SUBCASE("#")
        {
            magix::compile::SrcView src = U"#";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("#123")
        {
            magix::compile::SrcView src = U"#123";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 1},
                    {0, 4},
                    U"123",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("#+123")
        {
            magix::compile::SrcView src = U"#+123";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 1},
                    {0, 5},
                    U"+123",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("#-123,")
        {
            magix::compile::SrcView src = U"#-123,";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 1},
                    {0, 5},
                    U"-123",
                },
                {
                    magix::compile::TokenType::COMMA,
                    {0, 5},
                    {0, 6},
                    U",",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("#label ,")
        {
            magix::compile::SrcView src = U"#label ,";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::compile::TokenType::COMMA,
                    {0, 7},
                    {0, 8},
                    U",",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("#label,#1,#+2")
        {
            magix::compile::SrcView src = U"#label,#1,#+2";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::compile::TokenType::COMMA,
                    {0, 6},
                    {0, 7},
                    U",",
                },
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 7},
                    {0, 8},
                    U"#",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 8},
                    {0, 9},
                    U"1",
                },
                {
                    magix::compile::TokenType::COMMA,
                    {0, 9},
                    {0, 10},
                    U",",
                },
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 10},
                    {0, 11},
                    U"#",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 11},
                    {0, 13},
                    U"+2",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("Registers")
    {
        SUBCASE("$")
        {
            magix::compile::SrcView src = U"$";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::REGISTER_MARKER,
                    {0, 0},
                    {0, 1},
                    U"$",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("$123")
        {
            magix::compile::SrcView src = U"$123";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::REGISTER_MARKER,
                    {0, 0},
                    {0, 1},
                    U"$",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 1},
                    {0, 4},
                    U"123",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("$+123")
        {
            magix::compile::SrcView src = U"$+123";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::REGISTER_MARKER,
                    {0, 0},
                    {0, 1},
                    U"$",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 1},
                    {0, 5},
                    U"+123",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("$-123,")
        {
            magix::compile::SrcView src = U"$-123,";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::REGISTER_MARKER,
                    {0, 0},
                    {0, 1},
                    U"$",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 1},
                    {0, 5},
                    U"-123",
                },
                {
                    magix::compile::TokenType::COMMA,
                    {0, 5},
                    {0, 6},
                    U",",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("#label,$1,$+2")
        {
            magix::compile::SrcView src = U"#label,$1,$+2";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::compile::TokenType::COMMA,
                    {0, 6},
                    {0, 7},
                    U",",
                },
                {
                    magix::compile::TokenType::REGISTER_MARKER,
                    {0, 7},
                    {0, 8},
                    U"$",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 8},
                    {0, 9},
                    U"1",
                },
                {
                    magix::compile::TokenType::COMMA,
                    {0, 9},
                    {0, 10},
                    U",",
                },
                {
                    magix::compile::TokenType::REGISTER_MARKER,
                    {0, 10},
                    {0, 11},
                    U"$",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 11},
                    {0, 13},
                    U"+2",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("Directives")
    {
        SUBCASE(".")
        {
            magix::compile::SrcView src = U".";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE(". i32 16")
        {
            magix::compile::SrcView src = U". i32 16";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 5},
                    U"i32",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 6},
                    {0, 8},
                    U"16",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE(". align 16")
        {
            magix::compile::SrcView src = U". align 16";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 7},
                    U"align",
                },
                {
                    magix::compile::TokenType::NUMBER,
                    {0, 8},
                    {0, 10},
                    U"16",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("ident and dots")
    {
        SUBCASE(".f32")
        {
            magix::compile::SrcView src = U".f32";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 4},
                    U"f32",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("add.u32.imm")
        {
            magix::compile::SrcView src = U"add.u32.imm";

            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 11},
                    U"add.u32.imm",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE(".add.u32.imm")
        {
            // does not make sense to put instruction after dot, but still, identifiers, or what i call identifier, can contain but not
            // start with .
            magix::compile::SrcView src = U".add.u32.imm";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 12},
                    U"add.u32.imm",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("...add.u32.imm")
        {
            magix::compile::SrcView src = U"...add.u32.imm";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 1},
                    {0, 2},
                    U".",
                },
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 2},
                    {0, 3},
                    U".",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 3},
                    {0, 14},
                    U"add.u32.imm",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE(".add.u32.imm .")
        {
            magix::compile::SrcView src = U".add.u32.imm .";
            std::vector<magix::compile::SrcToken> got = magix::compile::lex(src);
            std::vector<magix::compile::SrcToken> expected = {
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::compile::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 12},
                    U"add.u32.imm",
                },
                {
                    magix::compile::TokenType::DIRECTIVE_MARKER,
                    {0, 13},
                    {0, 14},
                    U".",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
}

#endif
