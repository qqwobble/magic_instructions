#ifndef MAGIX_SPAN_HPP_
#define MAGIX_SPAN_HPP_

#include <cstddef>
#include <initializer_list>
#include <type_traits>

#include "magix_vm/meta.hpp"
#include "magix_vm/ranges.hpp"

namespace magix
{

template <class T>
class span
{
  public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using pointer = T *;
    using const_pointer = const T *;
    using iterator_type = pointer;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    constexpr span() noexcept = default;
    constexpr span(T *pointer, size_type size) noexcept : ptr{pointer}, length{size} {}

    template <std::size_t N>
    constexpr span(element_type (&arr)[N]) noexcept : ptr{&arr[0]}, length{N}
    {}

    template <class ItBeg, class SentEnt>
    constexpr span(ItBeg it, SentEnt end) noexcept : ptr{magix::ranges::to_address(it)}, length(end - it)
    {}
    template <class Range>
    constexpr span(Range &&range) : span(magix::ranges::begin(range), magix::ranges::end(range))
    {}
    constexpr span(std::initializer_list<T> init_list) : span(init_list.begin(), init_list.size()) {}

    constexpr span(const span &) = default;

    constexpr auto
    operator[](size_type i) const noexcept -> element_type &
    {
        return ptr[i];
    }

    [[nodiscard]] constexpr auto
    data() const noexcept -> pointer
    {
        return ptr;
    }

    [[nodiscard]] constexpr auto
    begin() const noexcept -> pointer
    {
        return ptr;
    }

    [[nodiscard]] constexpr auto
    end() const noexcept -> pointer
    {
        return ptr + length;
    }

    [[nodiscard]] constexpr auto
    size() const noexcept -> size_type
    {
        return length;
    }

    [[nodiscard]] constexpr auto
    as_const() const noexcept -> span<const T>
    {
        return *this;
    }

    [[nodiscard]] constexpr auto
    front() const noexcept -> element_type &
    {
        return ptr[0];
    }

    [[nodiscard]] constexpr auto
    back() const noexcept -> element_type &
    {
        return ptr[length - 1];
    }

    template <class U>
    [[nodiscard]] auto
    reinterpret() const noexcept -> std::enable_if_t<sizeof(T) == sizeof(U), span<U>>
    {
        return span<U>{reinterpret_cast<U *>(ptr), length};
    }

    template <class U>
    [[nodiscard]] auto
    reinterpret_resize() const noexcept -> span<U>
    {
        const size_t byte_size = length * sizeof(T);
        return span<U>{reinterpret_cast<U *>(ptr), byte_size / sizeof(U)};
    }

    [[nodiscard]] auto
    as_bytes() const noexcept -> span<copy_const_to_t<std::byte, T>>
    {
        using byte_type = copy_const_to_t<std::byte, T>;
        return span<byte_type>(reinterpret_cast<byte_type *>(ptr), length * sizeof(T));
    }

    [[nodiscard]] auto
    as_const_bytes() const noexcept -> span<const std::byte>
    {
        return span<const std::byte>{reinterpret_cast<const std::byte *>(ptr), length * sizeof(T)};
    }

    template <size_t N>
    [[nodiscard]] constexpr auto
    first() -> span
    {
        return {ptr, N};
    }

    [[nodiscard]] constexpr auto
    first(size_t n) -> span
    {
        return {ptr, n};
    }

    [[nodiscard]] constexpr auto
    subspan(size_t start, size_t end) -> span
    {
        return {ptr + start, end - start};
    }

  private:
    T *ptr = nullptr;
    size_type length = 0;
};
template <class ItBeg, class SentEnt>
span(ItBeg, SentEnt) -> span<std::remove_reference_t<ranges::iter_reference_t<ItBeg>>>;
template <class T, size_t N>
span(T (&)[N]) -> span<T>;
template <class Range>
span(Range &&) -> span<std::remove_reference_t<ranges::range_reference_t<Range>>>;

} // namespace magix

#endif // MAGIX_SPAN_HPP_
