#ifndef MAGIX_UTILITY_HPP_
#define MAGIX_UTILITY_HPP_

#include <functional>
#include <utility>

namespace magix
{

struct pair_hash
{
    template <class T1, class T2>
    auto
    operator()(const std::pair<T1, T2> &p) const -> std::size_t
    {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        // Mainly for demonstration purposes, i.e. works but is overly simple
        // In the real world, use sth. like boost.hash_combine
        return h1 ^ h2;
    }
};

template <class T, size_t N>
auto
array_size(T (&)[N]) -> size_t
{
    return N;
}

} // namespace magix

#endif // MAGIX_UTILITY_HPP_
