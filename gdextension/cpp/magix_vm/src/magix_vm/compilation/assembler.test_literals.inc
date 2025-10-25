TEST_SUITE("assembler/parse/literal")
{
// This test suite tests, if all numerical literal values are working as intended.
#define TEST_LITERAL_PARSE_OK(_type, _value)                                                                                               \
    SUBCASE(#_type ":" #_value)                                                                                                            \
    {                                                                                                                                      \
        ErrorStack error_stack;                                                                                                            \
        magix::compile::SrcView literal = U## #_value;                                                                                     \
        magix::compile::SrcToken num_token{                                                                                                \
            magix::compile::TokenType::NUMBER,                                                                                             \
            magix::compile::SrcLoc{0, 0},                                                                                                  \
            magix::compile::SrcLoc{0, literal.size()},                                                                                     \
            literal,                                                                                                                       \
        };                                                                                                                                 \
        _type a = -1;                                                                                                                      \
        bool ok = extract_number(num_token, a, error_stack);                                                                               \
        CHECK(ok);                                                                                                                         \
        const auto &errors = error_stack;                                                                                                  \
        std::array<magix::compile::AssemblerError, 0> expected;                                                                            \
        CHECK_RANGE_EQ(errors, expected);                                                                                                  \
        CHECK_EQ(a, _value);                                                                                                               \
    }

#define TEST_LITERAL_PARSE_ERR(_type, _value, _err_type)                                                                                   \
    SUBCASE(#_type ":" #_value)                                                                                                            \
    {                                                                                                                                      \
        ErrorStack error_stack;                                                                                                            \
        magix::compile::SrcView literal = U## #_value;                                                                                     \
        magix::compile::SrcToken num_token{                                                                                                \
            magix::compile::TokenType::NUMBER,                                                                                             \
            magix::compile::SrcLoc{0, 0},                                                                                                  \
            magix::compile::SrcLoc{0, literal.size()},                                                                                     \
            literal,                                                                                                                       \
        };                                                                                                                                 \
        _type a = -1;                                                                                                                      \
        bool ok = extract_number(num_token, a, error_stack);                                                                               \
        CHECK(!ok);                                                                                                                        \
        const auto &errors = error_stack;                                                                                                  \
        magix::compile::AssemblerError expected[] = {                                                                                      \
            _err_type{                                                                                                                     \
                num_token,                                                                                                                 \
            },                                                                                                                             \
        };                                                                                                                                 \
        CHECK_RANGE_EQ(errors, expected);                                                                                                  \
    }

#define TESTCASE_LITERAL_PARSE_ALL_OK(_value)                                                                                              \
    TEST_CASE(#_value)                                                                                                                     \
    {                                                                                                                                      \
        TEST_LITERAL_PARSE_OK(magix::i8, _value);                                                                                          \
        TEST_LITERAL_PARSE_OK(magix::i16, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::i32, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::i64, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::u8, _value);                                                                                          \
        TEST_LITERAL_PARSE_OK(magix::u16, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::u32, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::u64, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::f32, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::f64, _value);                                                                                         \
    }

#define TESTCASE_LITERAL_PARSE_SIGNED_OK(_value, _err_type)                                                                                \
    TEST_CASE(#_value)                                                                                                                     \
    {                                                                                                                                      \
        TEST_LITERAL_PARSE_OK(magix::i8, _value);                                                                                          \
        TEST_LITERAL_PARSE_OK(magix::i16, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::i32, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::i64, _value);                                                                                         \
        TEST_LITERAL_PARSE_ERR(magix::u8, _value, _err_type);                                                                              \
        TEST_LITERAL_PARSE_ERR(magix::u16, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::u32, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::u64, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_OK(magix::f32, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::f64, _value);                                                                                         \
    }

#define TESTCASE_LITERAL_PARSE_FLOAT_OK(_value, _err_type)                                                                                 \
    TEST_CASE(#_value)                                                                                                                     \
    {                                                                                                                                      \
        TEST_LITERAL_PARSE_ERR(magix::i8, _value, _err_type);                                                                              \
        TEST_LITERAL_PARSE_ERR(magix::i16, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::i32, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::i64, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::i8, _value, _err_type);                                                                              \
        TEST_LITERAL_PARSE_ERR(magix::u16, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::u32, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::u64, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_OK(magix::f32, _value);                                                                                         \
        TEST_LITERAL_PARSE_OK(magix::f64, _value);                                                                                         \
    }
#define TESTCASE_LITERAL_PARSE_NONE_OK(_value, _err_type)                                                                                  \
    TEST_CASE(#_value)                                                                                                                     \
    {                                                                                                                                      \
        TEST_LITERAL_PARSE_ERR(magix::i8, _value, _err_type);                                                                              \
        TEST_LITERAL_PARSE_ERR(magix::i16, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::i32, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::i64, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::i8, _value, _err_type);                                                                              \
        TEST_LITERAL_PARSE_ERR(magix::u16, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::u32, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::u64, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::f32, _value, _err_type);                                                                             \
        TEST_LITERAL_PARSE_ERR(magix::f64, _value, _err_type);                                                                             \
    }

    // basic integers
    TESTCASE_LITERAL_PARSE_ALL_OK(0);
    TESTCASE_LITERAL_PARSE_ALL_OK(1);
    TESTCASE_LITERAL_PARSE_ALL_OK(2);
    TESTCASE_LITERAL_PARSE_ALL_OK(3);
    // invalid chars
    TESTCASE_LITERAL_PARSE_NONE_OK(0a3, magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(+0hello3, magix::compile::assembler_errors::NumberInvalid);
    // plus minus
    TESTCASE_LITERAL_PARSE_ALL_OK(+1);
    TESTCASE_LITERAL_PARSE_SIGNED_OK(-1, magix::compile::assembler_errors::NumberNotRepresentable);
    // too many +-
    TESTCASE_LITERAL_PARSE_NONE_OK(++0, magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(+-0, magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(-+0, magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(--0, magix::compile::assembler_errors::NumberInvalid);
    // hex
    TESTCASE_LITERAL_PARSE_ALL_OK(0x0);
    TESTCASE_LITERAL_PARSE_ALL_OK(0x1);
    TESTCASE_LITERAL_PARSE_ALL_OK(0xA);
    TESTCASE_LITERAL_PARSE_ALL_OK(0xF);
    TESTCASE_LITERAL_PARSE_ALL_OK(+0x1);
    TESTCASE_LITERAL_PARSE_SIGNED_OK(-0x1, magix::compile::assembler_errors::NumberNotRepresentable);
    // invalid hex
    TESTCASE_LITERAL_PARSE_NONE_OK(0xg, magix::compile::assembler_errors::NumberInvalid);
    // float
    TESTCASE_LITERAL_PARSE_FLOAT_OK(0.0, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(0.5, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(1.0, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(.5, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(5., magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+0.5, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-0.5, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+.5, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-.5, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+5., magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-5., magix::compile::assembler_errors::NumberNotRepresentable);
    // float with exp
    TESTCASE_LITERAL_PARSE_FLOAT_OK(1.0e3, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+1.0e3, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-1.0e3, magix::compile::assembler_errors::NumberNotRepresentable);
    // invalid float
    TESTCASE_LITERAL_PARSE_NONE_OK(0.0.0, magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(..0, magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(.0., magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(0.., magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(.., magix::compile::assembler_errors::NumberInvalid);
    TESTCASE_LITERAL_PARSE_NONE_OK(1e1e1, magix::compile::assembler_errors::NumberInvalid);
    // hex float
    TESTCASE_LITERAL_PARSE_FLOAT_OK(0x1p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+0x1p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-0x1p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(0x1.0p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+0x1.0p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-0x1.0p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(0x1.p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+0x1.p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-0x1.p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(0x.0p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(+0x.0p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    TESTCASE_LITERAL_PARSE_FLOAT_OK(-0x.0p-1, magix::compile::assembler_errors::NumberNotRepresentable);
    // i8 range
    TEST_CASE("i8range")
    {
        TEST_LITERAL_PARSE_OK(magix::i8, 0x7F);
        TEST_LITERAL_PARSE_ERR(magix::i8, 0xFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i8, 0x7FFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i8, 0xFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i8, 0x7FFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i8, 0xFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i8, 0x7FFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i8, 0xFFFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
    }
    TEST_CASE("i16range")
    {
        TEST_LITERAL_PARSE_OK(magix::i16, 0x7F);
        TEST_LITERAL_PARSE_OK(magix::i16, 0xFF);
        TEST_LITERAL_PARSE_OK(magix::i16, 0x7FFF);
        TEST_LITERAL_PARSE_ERR(magix::i16, 0xFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i16, 0x7FFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i16, 0xFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i16, 0x7FFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i16, 0xFFFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
    }
    TEST_CASE("i32range")
    {
        TEST_LITERAL_PARSE_OK(magix::i32, 0x7F);
        TEST_LITERAL_PARSE_OK(magix::i32, 0xFF);
        TEST_LITERAL_PARSE_OK(magix::i32, 0x7FFF);
        TEST_LITERAL_PARSE_OK(magix::i32, 0xFFFF);
        TEST_LITERAL_PARSE_OK(magix::i32, 0x7FFFFFFF);
        TEST_LITERAL_PARSE_ERR(magix::i32, 0xFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i32, 0x7FFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::i32, 0xFFFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
    }
    TEST_CASE("i64range")
    {
        TEST_LITERAL_PARSE_OK(magix::i64, 0x7F);
        TEST_LITERAL_PARSE_OK(magix::i64, 0xFF);
        TEST_LITERAL_PARSE_OK(magix::i64, 0x7FFF);
        TEST_LITERAL_PARSE_OK(magix::i64, 0xFFFF);
        TEST_LITERAL_PARSE_OK(magix::i64, 0x7FFFFFFF);
        TEST_LITERAL_PARSE_OK(magix::i64, 0xFFFFFFFF);
        TEST_LITERAL_PARSE_OK(magix::i64, 0x7FFFFFFFFFFFFFFF);
        TEST_LITERAL_PARSE_ERR(magix::i64, 0xFFFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
    }
    TEST_CASE("u8range")
    {
        TEST_LITERAL_PARSE_OK(magix::u8, 0x7F);
        TEST_LITERAL_PARSE_OK(magix::u8, 0xFF);
        TEST_LITERAL_PARSE_ERR(magix::u8, 0x7FFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u8, 0xFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u8, 0x7FFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u8, 0xFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u8, 0x7FFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u8, 0xFFFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
    }
    TEST_CASE("u16range")
    {
        TEST_LITERAL_PARSE_OK(magix::u16, 0x7F);
        TEST_LITERAL_PARSE_OK(magix::u16, 0xFF);
        TEST_LITERAL_PARSE_OK(magix::u16, 0x7FFF);
        TEST_LITERAL_PARSE_OK(magix::u16, 0xFFFF);
        TEST_LITERAL_PARSE_ERR(magix::u16, 0x7FFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u16, 0xFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u16, 0x7FFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u16, 0xFFFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
    }
    TEST_CASE("u32range")
    {
        TEST_LITERAL_PARSE_OK(magix::u32, 0x7F);
        TEST_LITERAL_PARSE_OK(magix::u32, 0xFF);
        TEST_LITERAL_PARSE_OK(magix::u32, 0x7FFF);
        TEST_LITERAL_PARSE_OK(magix::u32, 0xFFFF);
        TEST_LITERAL_PARSE_OK(magix::u32, 0x7FFFFFFF);
        TEST_LITERAL_PARSE_OK(magix::u32, 0xFFFFFFFF);
        TEST_LITERAL_PARSE_ERR(magix::u32, 0x7FFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
        TEST_LITERAL_PARSE_ERR(magix::u32, 0xFFFFFFFFFFFFFFFF, magix::compile::assembler_errors::NumberNotRepresentable);
    }
    TEST_CASE("u64range")
    {
        TEST_LITERAL_PARSE_OK(magix::u64, 0x7F);
        TEST_LITERAL_PARSE_OK(magix::u64, 0xFF);
        TEST_LITERAL_PARSE_OK(magix::u64, 0x7FFF);
        TEST_LITERAL_PARSE_OK(magix::u64, 0xFFFF);
        TEST_LITERAL_PARSE_OK(magix::u64, 0x7FFFFFFF);
        TEST_LITERAL_PARSE_OK(magix::u64, 0xFFFFFFFF);
        TEST_LITERAL_PARSE_OK(magix::u64, 0x7FFFFFFFFFFFFFFF);
        TEST_LITERAL_PARSE_OK(magix::u64, 0xFFFFFFFFFFFFFFFF);
    }
}
