#ifndef MAGIX_DOCTEST_HELPER_HPP_
#define MAGIX_DOCTEST_HELPER_HPP_

#include "magix_vm/span.hpp"
#include <functional>
#include <iterator>

#ifdef MAGIX_BUILD_TESTS
#include <doctest.h>

namespace magix::_detail
{

template <class ItA, class SentA, class ItB, class SentB, class Cmp = std::equal_to<>>
auto
doctest_range_eq_impl(
    const char *file,
    int line,
    const doctest::String &name_a,
    const doctest::String &name_b,
    ItA it_a,
    SentA sent_a,
    ItB it_b,
    SentB sent_b,
    Cmp cmp = {}
) -> bool
{
    size_t index = 0;
    bool ok = true;
    while (it_a != sent_a && it_b != sent_b)
    {
        const auto &lhs = *it_a;
        const auto &rhs = *it_b;
        if (!cmp(lhs, rhs))
        {
            DOCTEST_INFO(name_a, "[", index, "] := ", lhs);
            DOCTEST_INFO(name_b, "[", index, "] := ", rhs);
            DOCTEST_ADD_FAIL_CHECK_AT(file, line, name_a, "[", index, "] != ", name_b, "[", index, "]");
            ok = false;
        }
        ++index;
        ++it_a;
        ++it_b;
    }
    while (it_a != sent_a)
    {
        const auto &lhs = *it_a;
        DOCTEST_INFO(name_a, "[", index, "] := ", lhs);
        DOCTEST_INFO(name_b, "[", index, "] := < OUT OF BOUNDS >");
        DOCTEST_ADD_FAIL_CHECK_AT(file, line, name_a, "[", index, "] != ", name_b, "[", index, "]");
        ok = false;
        ++index;
        ++it_a;
    }
    while (it_b != sent_b)
    {
        const auto &rhs = *it_b;
        DOCTEST_INFO(name_a, "[", index, "] := < OUT OF BOUNDS >");
        DOCTEST_INFO(name_b, "[", index, "] := ", rhs);
        DOCTEST_ADD_FAIL_CHECK_AT(file, line, name_a, "[", index, "] != ", name_b, "[", index, "]");
        ok = false;
        ++index;
        ++it_b;
    }
    return ok;
}

template <class RA, class RB, class Cmp = std::equal_to<>>
auto
doctest_range_eq_impl(
    const char *file,
    int line,
    const doctest::String &name_a,
    const doctest::String &name_b,
    const RA &ra,
    const RB &rb,
    Cmp cmp = {}
) -> bool
{
    using std::begin;
    using std::end;
    return doctest_range_eq_impl(file, line, name_a, name_b, begin(ra), end(ra), begin(rb), end(rb), std::move(cmp));
}

#define CHECK_RANGE_EQ(_r1, _r2, ...) ::magix::_detail::doctest_range_eq_impl(__FILE__, __LINE__, #_r1, #_r2, _r1, _r2, ##__VA_ARGS__)

auto
doctest_bytestring_eq_impl(
    const char *file,
    size_t line,
    const doctest::String &name_a,
    const doctest::String &name_b,
    magix::span<const std::byte> bytes_a,
    magix::span<const std::byte> bytes_b
) -> bool;

#define CHECK_BYTESTRING_EQ(_r1, _r2) ::magix::_detail::doctest_bytestring_eq_impl(__FILE__, __LINE__, #_r1, #_r2, _r1, _r2)

} // namespace magix::_detail

#endif

#endif // MAGIX_DOCTEST_HELPER_HPP_
