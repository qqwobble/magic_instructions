#ifndef MAGIX_TYPES_HPP_
#define MAGIX_TYPES_HPP_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace magix
{

using f32 = float;
static_assert(sizeof(f32) == 4);
using f64 = double;
static_assert(sizeof(f64) == 8);

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using b8 = uint8_t;
using b16 = uint16_t;
using b32 = uint32_t;
using b64 = uint64_t;

using code_word = u16;
constexpr code_word invalid_opcode = 0;

template <class T>
struct word_info;

template <>
struct word_info<u8>
{
    constexpr static size_t size = 1;
    constexpr static size_t align = 1;
    static_assert(size == sizeof(u8));
    static_assert(align >= alignof(u8));
};

template <>
struct word_info<u16>
{
    constexpr static size_t size = 2;
    constexpr static size_t align = 2;
    static_assert(size == sizeof(u16));
    static_assert(align >= alignof(u16));
};

template <>
struct word_info<u32>
{
    constexpr static size_t size = 4;
    constexpr static size_t align = 4;
    static_assert(size == sizeof(u32));
    static_assert(align >= alignof(u32));
};

template <>
struct word_info<u64>
{
    constexpr static size_t size = 8;
    constexpr static size_t align = 8;
    static_assert(size == sizeof(u64));
    static_assert(align >= alignof(u64));
};

template <>
struct word_info<i8>
{
    constexpr static size_t size = 1;
    constexpr static size_t align = 1;
    static_assert(size == sizeof(u8));
    static_assert(align >= alignof(u8));
};

template <>
struct word_info<i16>
{
    constexpr static size_t size = 2;
    constexpr static size_t align = 2;
    static_assert(size == sizeof(u16));
    static_assert(align >= alignof(u16));
};

template <>
struct word_info<i32>
{
    constexpr static size_t size = 4;
    constexpr static size_t align = 4;
    static_assert(size == sizeof(u32));
    static_assert(align >= alignof(u32));
};

template <>
struct word_info<i64>
{
    constexpr static size_t size = 8;
    constexpr static size_t align = 8;
    static_assert(size == sizeof(u64));
    static_assert(align >= alignof(u64));
};

template <>
struct word_info<f32>
{
    constexpr static size_t size = 4;
    constexpr static size_t align = 4;
    static_assert(size == sizeof(f32));
    static_assert(align >= alignof(f32));
};

template <>
struct word_info<f64>
{
    constexpr static size_t size = 8;
    constexpr static size_t align = 8;
    static_assert(size == sizeof(f64));
    static_assert(align >= alignof(f64));
};

/** Platform independent word alignment. */
template <class T>
constexpr auto code_align_v = word_info<T>::align;

/** Platform independent word size. */
template <class T>
constexpr auto code_size_v = word_info<T>::size;

/** Standard conform wrapping conversion. */
template <class T>
[[nodiscard]] constexpr auto
to_signed(T in) noexcept -> std::make_signed_t<T>
{
    // all of this ... maps to a register move.
    if constexpr (std::is_signed_v<T>)
    {
        return in;
    }
    using Signed = std::make_signed_t<T>;
    constexpr T max = std::numeric_limits<Signed>::max();
    if (in <= max)
    {
        return static_cast<Signed>(in);
    }
    else
    {
        // x = -(-x) = -(~x+1) = -(~x)-1
        // last step every intermediate in signed range
        return static_cast<Signed>(-static_cast<Signed>(~in) - 1);
    }
}

/** Adjusts signedness to match target type. */
template <class T, class U>
[[nodiscard]] constexpr auto
convert_signedness(U in) noexcept -> std::enable_if_t<std::is_same_v<std::make_unsigned_t<T>, std::make_unsigned_t<U>>, T>
{
    if constexpr (std::is_unsigned_v<T>)
    {
        return in;
    }
    else
    {
        return to_signed(in);
    }
}

} // namespace magix

#endif // MAGIX_TYPES_HPP_
