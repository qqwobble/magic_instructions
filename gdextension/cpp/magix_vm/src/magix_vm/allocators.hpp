#ifndef MAGIX_ALLOCATORS_HPP_
#define MAGIX_ALLOCATORS_HPP_

#include <cstddef>
#include <limits>
#include <new>

namespace magix
{

template <typename T, std::size_t align_size = 64>
class AlignedAllocator
{
  private:
    static_assert(align_size >= alignof(T), "alignment not sufficient for type");

  public:
    using value_type = T;
    static std::align_val_t constexpr alignment{align_size};

    template <class OtherElementType>
    struct rebind
    {
        using other = AlignedAllocator<OtherElementType, align_size>;
    };

  public:
    constexpr AlignedAllocator() noexcept = default;
    constexpr AlignedAllocator(const AlignedAllocator &) noexcept = default;
    template <typename U>
    constexpr AlignedAllocator(AlignedAllocator<U, align_size> const &) noexcept
    {}

    [[nodiscard]] auto
    allocate(std::size_t nElementsToAllocate) -> T *
    {
        if (nElementsToAllocate > std::numeric_limits<std::size_t>::max() / sizeof(T))
        {
            return nullptr;
            // throw std::bad_array_new_length();
        }

        auto const nBytesToAllocate = nElementsToAllocate * sizeof(T);
        return reinterpret_cast<T *>(::operator new[](nBytesToAllocate, alignment));
    }

    void
    deallocate(T *allocatedPointer, [[maybe_unused]] std::size_t nBytesAllocated)
    {
        ::operator delete[](allocatedPointer, alignment);
    }
};

} // namespace magix

#endif // MAGIX_ALLOCATORS_HPP_
