#ifndef MAGIX_TYPES_HPP_
#define MAGIX_TYPES_HPP_

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

template <class T>
[[nodiscard]] constexpr auto
to_signed(T in) noexcept -> std::make_signed_t<T>
{
    // all this is a standard conform and defined wrapping conversion
    // which just maps to a register move
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
        // two's complement magic
        return -static_cast<Signed>(~in) - 1;
    }
}

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
