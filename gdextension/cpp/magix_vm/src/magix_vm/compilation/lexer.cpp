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

[[nodiscard]] constexpr bool
is_number_start(magix::SrcChar begin)
{
    if (begin >= '0' && begin <= '9')
    {
        return true;
    }
    if (begin == '+' || begin == '-')
    {
        return true;
    }
    return false;
}

[[nodiscard]] constexpr bool
is_number_continue(magix::SrcChar begin)
{
    if (begin == '.' || begin == '+' || begin == '-')
    {
        // might appear somewhere in floats
        return true;
    }
    return godot::is_unicode_identifier_continue(begin);
}

struct Lexer
{
    using iterator = magix::SrcView::const_iterator;
    iterator it;
    iterator end;

    magix::SrcLoc current_loc = magix::SrcLoc::zero();

    Lexer(magix::SrcView src) : it(src.cbegin()), end(src.cend()) {}

    [[nodiscard]] magix::SrcToken
    make_eof()
    {
        magix::SrcView endview = {it, 0};
        return {
            magix::TokenType::END_OF_FILE,
            current_loc,
            current_loc,
            endview,
        };
    }

    [[nodiscard]] magix::SrcToken
    read_atomic_token(magix::TokenType type)
    {
        magix::SrcView view = {it++, 1};
        return {
            type,
            current_loc,
            current_loc.advance_column(),
            view,
        };
    }

    [[nodiscard]] magix::SrcToken
    read_identifier_token()
    {
        magix::SrcLoc begin_loc = current_loc;
        iterator ident_begin = it;

        while (it != end)
        {
            if (godot::is_unicode_identifier_continue(*it))
            {
                ++it;
                current_loc.advance_column();
            }
            else
            {
                break;
            }
        }

        magix::SrcLoc end_loc = current_loc;
        magix::SrcView view{ident_begin, (size_t)(it - ident_begin)};

        return {
            magix::TokenType::IDENTIFIER,
            begin_loc,
            end_loc,
            view,
        };
    }

    [[nodiscard]] magix::SrcToken
    read_string_token()
    {
        iterator begin_it = it;
        magix::SrcLoc begin_loc = current_loc;

        // skip opening "
        ++it;
        current_loc.advance_column();

        bool is_escaped = false;
        while (it != end)
        {
            magix::SrcChar read = *it++;
            if (read == magix::SYMBOL_NEWLINE)
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
                if (read == magix::SYMBOL_ESCAPE)
                {
                    is_escaped = true;
                }
                else if (read == magix::SYMBOL_QUOTES)
                {
                    magix::SrcView view{begin_it, (size_t)(it - begin_it)};
                    return {
                        magix::TokenType::STRING,
                        begin_loc,
                        current_loc,
                        view,
                    };
                }
            }
        }
        // not terminated
        magix::SrcView view{begin_it, (size_t)(it - begin_it)};
        return {
            magix::TokenType::UNTERMINATED_STRING,
            begin_loc,
            current_loc,
            view,
        };
    }

    [[nodiscard]] magix::SrcToken
    read_number_token()
    {
        magix::SrcLoc begin_loc = current_loc;
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

        magix::SrcLoc end_loc = current_loc;
        magix::SrcView view{ident_begin, (size_t)(it - ident_begin)};

        return {
            magix::TokenType::NUMBER,
            begin_loc,
            end_loc,
            view,
        };
    }

    void
    skip_comment()
    {
        auto old_it = it;
        it = std::find(it, end, magix::SYMBOL_NEWLINE);
        // does not eat the newline yet, as that is it's own token!
        current_loc.advance_column(it - old_it);
    }

    [[nodiscard]] magix::SrcToken
    next_token()
    {
    restart:
        if (it == end)
        {
            return make_eof();
        }

        magix::SrcChar first = *it;

        if (first == magix::SYMBOL_NEWLINE)
        {
            magix::SrcView view = {it++, 1};
            return {
                magix::TokenType::NEWLINE,
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
        if (first == magix::SYMBOL_COMMENT)
        {
            skip_comment();
            goto restart;
        }
        if (first == magix::SYMBOL_LABEL)
        {
            return read_atomic_token(magix::TokenType::LABEL_MARKER);
        }
        if (first == magix::SYMBOL_ENTRY)
        {
            return read_atomic_token(magix::TokenType::ENTRY_MARKER);
        }
        if (first == magix::SYMBOL_IMMEDIATE)
        {
            return read_atomic_token(magix::TokenType::IMMEDIATE_MARKER);
        }
        if (first == magix::SYMBOL_DIRECTIVE)
        {
            return read_atomic_token(magix::TokenType::DIRECTIVE_MARKER);
        }
        if (first == magix::SYMBOL_REGISTER)
        {
            return read_atomic_token(magix::TokenType::REGISTER_MARKER);
        }
        if (first == magix::SYMBOL_COMMA)
        {
            return read_atomic_token(magix::TokenType::COMMA);
        }
        if (first == magix::SYMBOL_QUOTES)
        {
            return read_string_token();
        }
        if (godot::is_unicode_identifier_start(first))
        {
            return read_identifier_token();
        }
        if (is_number_start(first))
        {
            return read_number_token();
        }

        // this char matches nothing!
        magix::SrcView view{it++, 1};
        return {
            magix::TokenType::INVALID_CHAR,
            current_loc,
            current_loc.advance_column(),
            view,
        };
    }
};

} // namespace

std::vector<magix::SrcToken>
magix::lex(SrcView source)
{
    Lexer lexer(source);
    std::vector<magix::SrcToken> out;

    while (true)
    {
        SrcToken token = lexer.next_token();
        out.push_back(token);
        if (token.type == TokenType::END_OF_FILE)
        {
            break;
        }
    }

    return out;
}

#ifdef MAGIX_BUILD_TESTS

TEST_SUITE("lexer")
{
    auto eof_line_end = [](magix::SrcView src) {
        return magix::SrcToken{
            magix::TokenType::END_OF_FILE,
            {0, src.length()},
            {0, src.length()},
            U"",
        };
    };
    auto eof_at = [](magix::SrcLoc loc) {
        return magix::SrcToken{
            magix::TokenType::END_OF_FILE,
            loc,
            loc,
            U"",
        };
    };
    TEST_CASE("no-token")
    {

        SUBCASE("empty")
        {
            magix::SrcView src = U"";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {eof_at({0, 0})};
            CHECK_RANGE_EQ(got, expected);
        }

        SUBCASE(";")
        {
            magix::SrcView src = U";";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {eof_at({0, 1})};
            CHECK_RANGE_EQ(got, expected);
        }
        SUBCASE("; foo")
        {
            magix::SrcView src = U"; foo";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {eof_at({0, 5})};
            CHECK_RANGE_EQ(got, expected);
        }
    }
    TEST_CASE("newlines")
    {
        SUBCASE("\\n")
        {
            magix::SrcView src = U"\n";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NEWLINE,
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
            magix::SrcView src = U"\n   ;   \n  ;";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NEWLINE,
                    {0, 0},
                    {1, 0},
                    U"\n",
                },
                {
                    magix::TokenType::NEWLINE,
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
            magix::SrcView src = U";\n;\n;\n;\n;\n;";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NEWLINE,
                    {0, 1},
                    {1, 0},
                    U"\n",
                },
                {
                    magix::TokenType::NEWLINE,
                    {1, 1},
                    {2, 0},
                    U"\n",
                },
                {
                    magix::TokenType::NEWLINE,
                    {2, 1},
                    {3, 0},
                    U"\n",
                },
                {
                    magix::TokenType::NEWLINE,
                    {3, 1},
                    {4, 0},
                    U"\n",
                },
                {
                    magix::TokenType::NEWLINE,
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
            magix::SrcView src = U"hello";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IDENTIFIER,
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
            magix::SrcView src = U"foo bar baz";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 3},
                    U"foo",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 4},
                    {0, 7},
                    U"bar",
                },
                {
                    magix::TokenType::IDENTIFIER,
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
            magix::SrcView src = U"foo bar ; baz";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 3},
                    U"foo",
                },
                {
                    magix::TokenType::IDENTIFIER,
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
            magix::SrcView src = U"ß α ඞ";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 0},
                    {0, 1},
                    U"ß",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 3},
                    U"α",
                },
                {
                    magix::TokenType::IDENTIFIER,
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
            magix::SrcView src = U"\"hello world!\"";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::STRING,
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
            magix::SrcView src = U"\"hello\" \"world!\"";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::STRING,
                    {0, 0},
                    {0, 7},
                    U"\"hello\"",
                },
                {
                    magix::TokenType::STRING,
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
            magix::SrcView src = U"\"hello ";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::UNTERMINATED_STRING,
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
            magix::SrcChar buffer[1];
            magix::SrcView src(buffer, 1);
            for (int dig = 0; dig < 10; ++dig)
            {
                CAPTURE(dig);
                buffer[0] = U'0' + dig;
                std::vector<magix::SrcToken> got = magix::lex(src);
                std::vector<magix::SrcToken> expected = {
                    {
                        magix::TokenType::NUMBER,
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
            magix::SrcChar buffer[2];
            magix::SrcView src(buffer, 2);
            for (int number = 0; number < 100; ++number)
            {
                CAPTURE(number);
                buffer[0] = U'0' + number / 10;
                buffer[1] = U'0' + number % 10;
                std::vector<magix::SrcToken> got = magix::lex(src);
                std::vector<magix::SrcToken> expected = {
                    {
                        magix::TokenType::NUMBER,
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
            magix::SrcChar buffer[3];
            buffer[1] = U'_';
            magix::SrcView src(buffer, 3);
            for (int number = 0; number < 100; ++number)
            {
                CAPTURE(number);
                buffer[0] = U'0' + number / 10;
                buffer[2] = U'0' + number % 10;
                std::vector<magix::SrcToken> got = magix::lex(src);
                std::vector<magix::SrcToken> expected = {
                    {
                        magix::TokenType::NUMBER,
                        {0, 0},
                        {0, 3},
                        src,
                    },
                    eof_line_end(src),
                };
                CHECK_RANGE_EQ(got, expected);
            }
        }

        auto dig2hex = [](int d) -> magix::SrcChar {
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
            magix::SrcChar buffer[4];
            buffer[0] = U'0';
            buffer[1] = U'x';
            magix::SrcView src(buffer, 4);
            for (int number = 0; number < 0x100; ++number)
            {
                CAPTURE(number);
                buffer[2] = dig2hex(number / 16);
                buffer[3] = dig2hex(number % 16);
                std::vector<magix::SrcToken> got = magix::lex(src);
                std::vector<magix::SrcToken> expected = {
                    {
                        magix::TokenType::NUMBER,
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
            magix::SrcView src = U"1.0";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"3.1415926535897932384626433";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"+1.0";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"-1.0";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"-1.0abc";
            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"@";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::ENTRY_MARKER,
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
            magix::SrcView src = U"@ label :";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::ENTRY_MARKER,
                    {0, 0},
                    {0, 1},
                    U"@",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 7},
                    U"label",
                },
                {
                    magix::TokenType::LABEL_MARKER,
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
            magix::SrcView src = U"@label:";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::ENTRY_MARKER,
                    {0, 0},
                    {0, 1},
                    U"@",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::TokenType::LABEL_MARKER,
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
            magix::SrcView src = U"#";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IMMEDIATE_MARKER,
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
            magix::SrcView src = U"#123";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"#+123";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"#-123,";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::TokenType::NUMBER,
                    {0, 1},
                    {0, 5},
                    U"-123",
                },
                {
                    magix::TokenType::COMMA,
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
            magix::SrcView src = U"#label ,";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::TokenType::COMMA,
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
            magix::SrcView src = U"#label,#1,#+2";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::TokenType::COMMA,
                    {0, 6},
                    {0, 7},
                    U",",
                },
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 7},
                    {0, 8},
                    U"#",
                },
                {
                    magix::TokenType::NUMBER,
                    {0, 8},
                    {0, 9},
                    U"1",
                },
                {
                    magix::TokenType::COMMA,
                    {0, 9},
                    {0, 10},
                    U",",
                },
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 10},
                    {0, 11},
                    U"#",
                },
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"$";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::REGISTER_MARKER,
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
            magix::SrcView src = U"$123";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::REGISTER_MARKER,
                    {0, 0},
                    {0, 1},
                    U"$",
                },
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"$+123";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::REGISTER_MARKER,
                    {0, 0},
                    {0, 1},
                    U"$",
                },
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U"$-123,";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::REGISTER_MARKER,
                    {0, 0},
                    {0, 1},
                    U"$",
                },
                {
                    magix::TokenType::NUMBER,
                    {0, 1},
                    {0, 5},
                    U"-123",
                },
                {
                    magix::TokenType::COMMA,
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
            magix::SrcView src = U"#label,$1,$+2";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::IMMEDIATE_MARKER,
                    {0, 0},
                    {0, 1},
                    U"#",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 1},
                    {0, 6},
                    U"label",
                },
                {
                    magix::TokenType::COMMA,
                    {0, 6},
                    {0, 7},
                    U",",
                },
                {
                    magix::TokenType::REGISTER_MARKER,
                    {0, 7},
                    {0, 8},
                    U"$",
                },
                {
                    magix::TokenType::NUMBER,
                    {0, 8},
                    {0, 9},
                    U"1",
                },
                {
                    magix::TokenType::COMMA,
                    {0, 9},
                    {0, 10},
                    U",",
                },
                {
                    magix::TokenType::REGISTER_MARKER,
                    {0, 10},
                    {0, 11},
                    U"$",
                },
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U".";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::DIRECTIVE_MARKER,
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
            magix::SrcView src = U". i32 16";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 5},
                    U"i32",
                },
                {
                    magix::TokenType::NUMBER,
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
            magix::SrcView src = U". align 16";

            std::vector<magix::SrcToken> got = magix::lex(src);
            std::vector<magix::SrcToken> expected = {
                {
                    magix::TokenType::DIRECTIVE_MARKER,
                    {0, 0},
                    {0, 1},
                    U".",
                },
                {
                    magix::TokenType::IDENTIFIER,
                    {0, 2},
                    {0, 7},
                    U"align",
                },
                {
                    magix::TokenType::NUMBER,
                    {0, 8},
                    {0, 10},
                    U"16",
                },
                eof_line_end(src),
            };
            CHECK_RANGE_EQ(got, expected);
        }
    }
}

#endif
