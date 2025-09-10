#ifndef MAGIX_VARIANT_HELPER_HPP_
#define MAGIX_VARIANT_HELPER_HPP_

namespace magix
{

template <class... T>
struct overload : T...
{
    using T::
    operator()...;
};
// guidelines necessary in c++17
template <class... T>
overload(T...) -> overload<T...>;

} // namespace magix

#endif // MAGIX_VARIANT_HELPER_HPP_
