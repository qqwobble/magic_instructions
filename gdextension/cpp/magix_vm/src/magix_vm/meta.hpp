#ifndef MAGIX_META_HPP_
#define MAGIX_META_HPP_

namespace magix
{

template <class Dst, class Src> struct copy_const_to
{
    using type = Dst;
};

template <class Dst, class Src> struct copy_const_to<Dst, const Src>
{
    using type = const Dst;
};

template <class Dst, class Src> using copy_const_to_t = typename copy_const_to<Dst, Src>::type;

} // namespace magix

#endif // MAGIX_META_HPP_
