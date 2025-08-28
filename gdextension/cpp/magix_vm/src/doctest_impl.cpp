#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <algorithm>
#include <streambuf>
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

namespace
{

class Stream2Godot : public std::streambuf
{
    std::basic_string<char_type> str;

  protected:
    void
    flush_line()
    {
        godot::print_line(godot::String(str.c_str()));
        str.clear();
    }

    std::streamsize
    xsputn(const char_type *s, std::streamsize n) override
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
            flush_line();
            beg = newline + 1;
        }
        return n;
    };

    int_type
    overflow(int_type ch) override
    {
        if (ch == '\n')
        {
            flush_line();
        }
        else
        {
            str.push_back(ch);
        }
        return 1;
    }
};

} // namespace

int
magix_run_doctest()
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
