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
