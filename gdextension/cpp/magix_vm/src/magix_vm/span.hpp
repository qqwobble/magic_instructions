#ifndef MAGIX_SPAN_HPP_
#define MAGIX_SPAN_HPP_

#include <cstddef>
#include <initializer_list>
#include <type_traits>

#include "magix_vm/meta.hpp"
#include "magix_vm/ranges.hpp"

namespace magix
{

template <class T> class span
{
  public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using pointer = T *;
    using const_pointer = const T *;
    using iterator_type = pointer;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    constexpr span() noexcept {}
    constexpr span(T *pointer, size_type size) noexcept : ptr{pointer}, length{size} {}

    template <std::size_t N> constexpr span(element_type (&arr)[N]) noexcept : ptr{&arr[0]}, length{N} {}

    template <class ItBeg, class SentEnt>
    constexpr span(ItBeg it, SentEnt end) noexcept : ptr{magix::ranges::to_address(it)}, length(end - it)
    {}
    template <class Range> constexpr span(Range &&range) : span(magix::ranges::begin(range), magix::ranges::end(range)) {}
    constexpr span(std::initializer_list<T> init_list) : span(init_list.begin(), init_list.size()) {}

    constexpr span(const span &) = default;

    constexpr element_type &
    operator[](size_type i) const noexcept
    {
        return ptr[i];
    }

    constexpr pointer
    data() const noexcept
    {
        return ptr;
    }

    constexpr pointer
    begin() const noexcept
    {
        return ptr;
    }

    constexpr pointer
    end() const noexcept
    {
        return ptr + length;
    }

    constexpr size_type
    size() const noexcept
    {
        return length;
    }

    [[nodiscard]] constexpr span<const T>
    as_const() const noexcept
    {
        return *this;
    }

    constexpr element_type &
    front() const noexcept
    {
        return ptr[0];
    }

    constexpr element_type &
    back() const noexcept
    {
        return ptr[length - 1];
    }

    template <class U>
    [[nodiscard]] auto
    reinterpret() const noexcept -> std::enable_if_t<sizeof(T) == sizeof(U), span<U>>
    {
        return span<U>{reinterpret_cast<U *>(ptr), length};
    }

    [[nodiscard]] span<copy_const_to_t<std::byte, T>>
    as_bytes() const noexcept
    {
        using byte_type = copy_const_to_t<std::byte, T>;
        return span<byte_type>(reinterpret_cast<byte_type *>(ptr), length * sizeof(T));
    }

    template <size_t N>
    [[nodiscard]] span
    first()
    {
        return {ptr, N};
    }

  private:
    T *ptr = nullptr;
    size_type length = 0;
};
template <class ItBeg, class SentEnt> span(ItBeg, SentEnt) -> span<std::remove_reference_t<ranges::iter_reference_t<ItBeg>>>;
template <class T, size_t N> span(T (&)[N]) -> span<T>;
template <class Range> span(Range &&) -> span<std::remove_reference_t<ranges::range_reference_t<Range>>>;

} // namespace magix

#endif // MAGIX_SPAN_HPP_
