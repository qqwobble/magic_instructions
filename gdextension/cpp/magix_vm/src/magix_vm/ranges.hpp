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
constexpr T *
to_address(T *ptr)
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

template <class It> using iter_reference_t = decltype(*std::declval<It &>());
template <class R> using range_iter_t = decltype(ranges::begin(*std::declval<R &>()));
template <class R> using range_reference_t = decltype(*ranges::begin(std::declval<R &>()));

template <class It, class Sent = It> class subrange
{
  public:
    constexpr subrange() = default;
    constexpr subrange(It it, Sent sentinel) : it{it}, sentinel{sentinel} {}

    constexpr It
    begin() const
    {
        return it;
    }

    constexpr Sent
    end() const
    {
        return sentinel;
    }

  private:
    It it;
    Sent sentinel;
};

} // namespace magix::ranges

#endif // MAGIX_RANGES_HPP_
