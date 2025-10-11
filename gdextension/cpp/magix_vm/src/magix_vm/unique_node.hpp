#ifndef MAGIX_UNIQUE_NODE_HPP_
#define MAGIX_UNIQUE_NODE_HPP_

#include "godot_cpp/core/memory.hpp"
#include <cstddef>

namespace magix
{
template <class T>
class UniqueNode
{
  public:
    constexpr UniqueNode() = default;
    constexpr UniqueNode(std::nullptr_t) {}
    explicit constexpr UniqueNode(T *node) : _node{node} {}

    UniqueNode(const UniqueNode &) = delete;
    auto
    operator=(const UniqueNode &) = delete;

    constexpr UniqueNode(UniqueNode &&from) noexcept : _node{from.release()} {}
    template <class U>
    constexpr UniqueNode(UniqueNode<U> &&from) noexcept : _node{from.release()}
    {}

    template <class U>
    constexpr auto
    operator=(UniqueNode<U> &&from) -> UniqueNode &
    {
        reset(from.release());
        return *this;
    }
    constexpr auto
    operator=(std::nullptr_t) -> UniqueNode &
    {
        reset();
        return *this;
    }

    [[nodiscard]] constexpr auto
    get() const noexcept
    {
        return _node;
    }

    [[nodiscard]] explicit constexpr
    operator bool() const noexcept
    {
        return _node != nullptr;
    }

    constexpr auto
    operator->() const noexcept -> T *
    {
        return _node;
    }

    constexpr auto
    operator*() const noexcept -> T &
    {
        return *_node;
    }

    [[nodiscard]] constexpr auto
    release() noexcept -> T *
    {
        T *ret = _node;
        _node = nullptr;
        return ret;
    }

    constexpr void
    reset(T *node = nullptr)
    {
        if (_node != nullptr)
        {
            godot::memdelete(_node);
        }
        _node = node;
    }

    ~UniqueNode()
    {
        reset();
    }

  private:
    T *_node = nullptr;
};

template <class T>
auto
make_unique_node() -> UniqueNode<T>
{
    return UniqueNode<T>{memnew(T)};
}
} // namespace magix

#endif // MAGIX_UNIQUE_NODE_HPP_
