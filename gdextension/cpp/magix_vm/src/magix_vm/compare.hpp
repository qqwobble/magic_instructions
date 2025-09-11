#ifndef MAGIX_COMPARE_HPP_
#define MAGIX_COMPARE_HPP_

#include <type_traits>
#include <utility>
namespace magix
{

namespace _detail
{
struct less_t
{
    template <class T, class U>
    [[nodiscard]] constexpr auto
    operator()(const T &lhs, const U &rhs) const -> bool
    {
        return lhs < rhs;
    }
};

struct safe_less_t
{
    template <class T, class U>
    [[nodiscard]] constexpr auto
    operator()(const T &lhs, const U &rhs) const -> std::enable_if_t<std::is_integral_v<T> && std::is_integral_v<U>, bool>
    {
        if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
        {
            return lhs < rhs;
        }
        else if constexpr (std::is_signed_v<T>)
        {
            if (lhs < 0)
            {
                return true;
            }
            else
            {
                return std::make_unsigned_t<T>(lhs) < rhs;
            }
        }
        else
        {
            if (rhs < 0)
            {
                return false;
            }
            else
            {
                return lhs < std::make_unsigned_t<U>(rhs);
            }
        }
    }
};
} // namespace _detail

constexpr _detail::less_t less;
constexpr _detail::safe_less_t safe_less;

template <class T, class U, class Cmp = _detail::less_t>
[[nodiscard]] auto
max(T lhs, U rhs, Cmp &&cmp = less) -> std::common_type_t<T, U>
{
    if (std::forward<Cmp>(cmp)(lhs, rhs))
    {
        return rhs;
    }
    else
    {
        return lhs;
    }
}

template <class T, class U, class Cmp = _detail::less_t>
[[nodiscard]] auto
min(T lhs, U rhs, Cmp &&cmp = {}) -> std::common_type_t<T, U>
{
    if (std::forward<Cmp>(cmp)(lhs, rhs))
    {
        return lhs;
    }
    else
    {
        return rhs;
    }
}
} // namespace magix

#endif // MAGIX_COMPARE_HPP_
