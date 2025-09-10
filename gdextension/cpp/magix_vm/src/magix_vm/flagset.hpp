#ifndef MAGIX_FLAGSET_HPP_
#define MAGIX_FLAGSET_HPP_

#include "magix_vm/types.hpp"

#include <initializer_list>
#include <type_traits>

namespace magix
{

template <class V, class S = V>
class SetBitIter
{
  public:
    using value_type = V;
    using storage_type = S;

    constexpr SetBitIter() = default;
    constexpr SetBitIter(value_type value) : _storage(static_cast<storage_type>(value)) {}
    template <class U = storage_type, typename = std::enable_if_t<!std::is_same_v<storage_type, value_type>>>
    constexpr SetBitIter(storage_type store) : _storage(store)
    {}

    constexpr auto
    operator*() const -> value_type
    {
        using unsigned_storage = std::make_unsigned_t<storage_type>;
        unsigned_storage old_value = _storage;
        unsigned_storage least_bit = old_value & (-old_value);
        storage_type temp = convert_signedness<storage_type>(least_bit);
        return static_cast<value_type>(temp);
    }

    constexpr auto
    operator++() noexcept -> SetBitIter &
    {
        using unsigned_storage = std::make_unsigned_t<storage_type>;
        unsigned_storage old_value = _storage;
        unsigned_storage least_cleared = old_value & (old_value - 1);
        _storage = convert_signedness<storage_type>(least_cleared);
        return *this;
    }
    constexpr auto
    operator++(int) noexcept -> SetBitIter
    {
        SetBitIter old = *this;
        ++*this;
        return old;
    }

    constexpr auto
    operator==(const SetBitIter &rhs) const noexcept -> bool
    {
        return _storage == rhs._storage;
    }
    constexpr auto
    operator!=(const SetBitIter &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }

  private:
    storage_type _storage;
};

template <class V, class S = V>
class FlagSet
{
  public:
    using value_type = V;
    using storage_type = S;

    using iterator = SetBitIter<V, S>;

    constexpr FlagSet() = default;
    constexpr FlagSet(value_type value) : _storage(static_cast<storage_type>(value)) {}
    template <class U = storage_type, typename = std::enable_if_t<!std::is_same_v<value_type, U>>>
    constexpr explicit FlagSet(storage_type storage) : _storage{storage}
    {}
    constexpr FlagSet(std::initializer_list<value_type> values) : _storage{}
    {
        for (const value_type &value : values)
        {
            _storage |= static_cast<storage_type>(value);
        }
    }

    constexpr explicit
    operator bool() const noexcept
    {
        return _storage;
    }

    constexpr explicit
    operator value_type() const noexcept
    {
        return static_cast<value_type>(_storage);
    }

    [[nodiscard]] constexpr auto
    contains(value_type flag) const noexcept -> bool
    {
        return bool{*this & flag};
    }

    [[nodiscard]] constexpr auto
    operator&(const FlagSet &rhs) const noexcept -> FlagSet
    {
        return FlagSet{_storage & rhs._storage};
    }

    constexpr auto
    operator&=(const FlagSet &rhs) const noexcept -> FlagSet &
    {
        _storage &= rhs._storage;
        return *this;
    }

    [[nodiscard]] constexpr auto
    operator|(const FlagSet &rhs) const noexcept -> FlagSet
    {
        return FlagSet{_storage | rhs._storage};
    }

    constexpr auto
    operator|=(const FlagSet &rhs) const noexcept -> FlagSet &
    {
        _storage |= rhs._storage;
        return *this;
    }

    [[nodiscard]] constexpr auto
    storage() const noexcept -> const storage_type &
    {
        return _storage;
    }

    [[nodiscard]] constexpr auto
    storage() noexcept -> storage_type &
    {
        return _storage;
    }

    [[nodiscard]] constexpr auto
    begin() const noexcept -> iterator
    {
        return iterator{_storage};
    }

    [[nodiscard]] constexpr auto
    end() const noexcept -> iterator
    {
        return iterator{storage_type{}};
    }

    constexpr auto
    operator==(const FlagSet &rhs) const noexcept -> bool
    {
        return _storage == rhs._storage;
    }

    constexpr auto
    operator!=(const FlagSet &rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }

  private:
    storage_type _storage;
};

template <class T>
using BitEnumSet = FlagSet<T, std::underlying_type_t<T>>;

#define MAGIX_DECLARE_BIT_ENUM_OPS(_enum_name)                                                                                             \
    [[nodiscard]] constexpr auto operator|(_enum_name lhs, _enum_name rhs) noexcept -> BitEnumSet<_enum_name>                              \
    {                                                                                                                                      \
        return BitEnumSet<_enum_name>{lhs} | BitEnumSet<_enum_name>{rhs};                                                                  \
    }                                                                                                                                      \
    [[nodiscard]] constexpr auto operator&(_enum_name lhs, _enum_name rhs) noexcept -> BitEnumSet<_enum_name>                              \
    {                                                                                                                                      \
        return BitEnumSet<_enum_name>{lhs} & BitEnumSet<_enum_name>{rhs};                                                                  \
    }

} // namespace magix

#endif // MAGIX_FLAGSET_HPP_
