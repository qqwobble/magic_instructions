#ifndef MAGIX_RANGES_HPP_
#define MAGIX_RANGES_HPP_

#include <iterator>
#include <utility>

namespace magix::_detail::ranges
{
template <class R>
constexpr auto
_begin(R &&r)
{
    using std::begin;
    return begin(r);
}

template <class R>
constexpr auto
_end(R &&r)
{
    using std::end;
    return end(r);
}
} // namespace magix::_detail::ranges

namespace magix::ranges
{

template <class T>
constexpr auto
to_address(T *ptr) -> T *
{
    return ptr;
}

template <class T>
constexpr auto
to_address(const T &t)
{
    return to_address(t.operator->());
}

template <class R>
constexpr auto
begin(R &&r)
{
    return magix::_detail::ranges::_begin(r);
}

template <class R>
constexpr auto
end(R &&r)
{
    return magix::_detail::ranges::_end(r);
}

template <class It>
using iter_reference_t = decltype(*std::declval<It &>());
template <class R>
using range_iter_t = decltype(ranges::begin(*std::declval<R &>()));
template <class R>
using range_reference_t = decltype(*ranges::begin(std::declval<R &>()));

template <class It, class Sent = It>
class subrange
{
  public:
    constexpr subrange() = default;
    constexpr subrange(It it, Sent sentinel) : it{it}, sentinel{sentinel} {}

    [[nodiscard]] constexpr auto
    begin() const -> It
    {
        return it;
    }

    [[nodiscard]] constexpr auto
    end() const -> Sent
    {
        return sentinel;
    }

  private:
    It it;
    Sent sentinel;
};

namespace _detail
{
struct reverse_view_type
{
    template <class It, class Sent>
    auto
    operator()(It it, Sent sent)
    {
        return subrange(std::make_reverse_iterator(sent), std::make_reverse_iterator(it));
    }
    template <class Range>
    auto
    operator()(Range &&range)
    {
        return operator()(magix::ranges::begin(range), magix::ranges::end(range));
    }
};

template <class Range>
auto
operator|(Range &&range, reverse_view_type rvt)
{
    return rvt(std::forward<Range>(range));
}
} // namespace _detail

constexpr _detail::reverse_view_type reverse_view;

template <class T = int>
class integral_iterator
{

  public:
    using difference_type = T;
    using value_type = T;

    constexpr integral_iterator() = default;
    constexpr integral_iterator(T value) : value{value} {}

    constexpr auto
    operator++() -> integral_iterator &
    {
        ++value;
        return *this;
    }

    constexpr auto
    operator++(int) -> integral_iterator
    {
        auto ret = *this;
        ++value;
        return ret;
    }

    constexpr auto
    operator+(T rhs) -> integral_iterator
    {
        return {value - rhs};
    }

    constexpr auto
    operator+=(T rhs) -> integral_iterator &
    {
        value += rhs;
        return *this;
    }

    constexpr auto
    operator--() -> integral_iterator &
    {
        --value;
        return *this;
    }

    constexpr auto
    operator--(int) -> integral_iterator
    {
        auto ret = *this;
        --value;
        return ret;
    }

    constexpr auto
    operator-(T rhs) -> integral_iterator
    {
        return {value - rhs};
    }

    constexpr auto
    operator-=(T rhs) -> integral_iterator &
    {
        value -= rhs;
        return *this;
    }

    [[nodiscard]] auto
    operator*() const noexcept -> const value_type &
    {
        return value;
    }

    [[nodiscard]] auto
    operator[](T off) const noexcept -> const value_type &
    {
        return value + off;
    }

    auto
    operator==(const integral_iterator &rhs) const noexcept -> bool
    {
        return value == rhs.value;
    }
    auto
    operator!=(const integral_iterator &rhs) const noexcept -> bool
    {
        return value != rhs.value;
    }
    auto
    operator<=(const integral_iterator &rhs) const noexcept -> bool
    {
        return value <= rhs.value;
    }
    auto
    operator<(const integral_iterator &rhs) const noexcept -> bool
    {
        return value < rhs.value;
    }
    auto
    operator>=(const integral_iterator &rhs) const noexcept -> bool
    {
        return value >= rhs.value;
    }
    auto
    operator>(const integral_iterator &rhs) const noexcept -> bool
    {
        return value > rhs.value;
    }

    constexpr auto
    operator-(const integral_iterator &rhs) const noexcept -> difference_type
    {
        return value - rhs.value;
    }

    [[nodiscard]] auto
    unsafe_get_ref() const noexcept -> value_type &
    {
        return value;
    }

  private:
    T value;
};

template <class T = int>
auto
num_range(T from, T to) -> subrange<integral_iterator<T>>
{
    return {integral_iterator<T>(from), integral_iterator<T>(to)};
}

template <class T = int>
auto
num_range(T to) -> subrange<integral_iterator<T>>
{
    return num_range<T>(0, to);
}

} // namespace magix::ranges

#endif // MAGIX_RANGES_HPP_
