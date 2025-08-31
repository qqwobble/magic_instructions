#ifndef MAGIX_BUILD_TESTS
#error TEST FILE INCLUDED IN NON TEST BUILD!
#endif

#include "magix_vm/ranges.hpp"

#include "magix_vm/doctest_helper.hpp"

TEST_SUITE("ranges/reverse")
{
    TEST_CASE("simple")
    {
        int src[] = {1, 2, 3, 4};
        int expect_arr[] = {4, 3, 2, 1};
        CHECK_RANGE_EQ(src | magix::ranges::reverse_view, expect_arr);
    }
}

TEST_SUITE("ranges/num")
{
    TEST_CASE("1..10")
    {
        int expected[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        CHECK_RANGE_EQ(magix::ranges::num_range(1, 11), expected);
    }
    TEST_CASE("integ rel")
    {
        magix::ranges::integral_iterator<> one(1);
        magix::ranges::integral_iterator<> one_(1);
        magix::ranges::integral_iterator<> two(2);
        CHECK_EQ(*one < *two, true);
        CHECK_EQ(*one <= *two, true);
        CHECK_EQ(*one > *two, false);
        CHECK_EQ(*one >= *two, false);
        CHECK_EQ(*one == *two, false);
        CHECK_EQ(*one != *two, true);

        CHECK_EQ(*one < *one_, false);
        CHECK_EQ(*one <= *one_, true);
        CHECK_EQ(*one > *one_, false);
        CHECK_EQ(*one >= *one_, true);
        CHECK_EQ(*one == *one_, true);
        CHECK_EQ(*one != *one_, false);
    }
}
