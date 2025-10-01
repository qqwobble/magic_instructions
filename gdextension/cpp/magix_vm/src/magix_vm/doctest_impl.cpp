#include <algorithm>
#include <sstream>
#include <streambuf>

#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/variant/string.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include "doctest_helper.hpp"

namespace
{

class Stream2Godot : public std::streambuf
{
    std::basic_string<char_type> str;

  protected:
    void
    flush_line(bool implicit_newline)
    {
        if (implicit_newline || !str.empty())
        {
            godot::print_line(godot::String(str.c_str()));
            str.clear();
        }
    }

    auto
    xsputn(const char_type *s, std::streamsize n) -> std::streamsize override
    {
        auto beg = s;
        auto end = s + n;
        while (beg != end)
        {
            auto newline = std::find(beg, end, '\n');
            if (newline == end)
            {
                // no newline
                str.append(beg, end);
                break;
            }
            str.append(beg, newline);
            flush_line(true);
            beg = newline + 1;
        }
        return n;
    };

    auto
    overflow(int_type ch) -> int_type override
    {
        if (traits_type::eq_int_type(ch, traits_type::eof()))
        {
            flush_line(false);
        }
        else if (traits_type::eq_int_type(ch, traits_type::to_int_type('\n')))
        {
            flush_line(false);
        }
        else
        {
            str.push_back(traits_type::to_char_type(ch));
        }
        return traits_type::not_eof(ch);
    }
};

} // namespace

auto
magix_run_doctest() -> int
{
    // straight up copied from the examples
    doctest::Context context;

    // example settins
    // context.addFilter("test-case-exclude", "*math*");
    // context.setOption("abort-after", 5);
    // context.setOption("order-by", "name");
    Stream2Godot outbuf;
    std::ostream outstrm(&outbuf);
    context.setCout(&outstrm);

    context.applyCommandLine(0, nullptr);

    int res = context.run(); // run

    if (context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;           // propagate the result of the tests

    // at this point we should do our program and report some combination of the exit codes.
    // but as we explicitly only want to run tests there is nothing to report back
    return res;
}

auto
magix::_detail::doctest_bytestring_eq_impl(
    const char *file,
    int line,
    const doctest::String &name_a,
    const doctest::String &name_b,
    magix::span<const std::byte> bytes_a,
    magix::span<const std::byte> bytes_b
) -> bool
{

    if (std::equal(bytes_a.begin(), bytes_a.end(), bytes_b.begin(), bytes_b.end()))
    {
        return true;
    }

    std::ostringstream stream_a;
    stream_a << std::setfill('0') << std::setw(2) << std::hex;
    for (std::byte byte : bytes_a)
    {
        stream_a << static_cast<unsigned>(byte) << ',';
    }

    std::ostringstream stream_b;
    stream_b << std::setfill('0') << std::setw(2) << std::hex;
    for (std::byte byte : bytes_b)
    {
        stream_b << static_cast<unsigned>(byte) << ',';
    }

    DOCTEST_INFO(name_a, ": ", stream_a.str());
    DOCTEST_INFO(name_b, ": ", stream_b.str());
    DOCTEST_ADD_FAIL_CHECK_AT(file, line, name_a, " != ", name_b);

    return false;
}
