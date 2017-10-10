#ifndef SUPERMALLOC_ALLOCATOR_H_
#define SUPERMALLOC_ALLOCATOR_H_

#include "supermalloc.h"
#include <new>
#include <type_traits>

template<typename T>
class supermalloc_allocator
{
    template<typename... Ts> constexpr static void maybe_unused(Ts&&...) noexcept {}
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    constexpr supermalloc_allocator() noexcept = default;

    template<typename U>
    constexpr supermalloc_allocator(supermalloc_allocator<U> const&) noexcept {}

    T* allocate(size_t n)
    {
        size_t alignment = alignof(T);
        size_t size      = n * sizeof(T);
        if (T* allocated = static_cast<T*>(supermalloc_aligned_alloc(alignment, size)))
            return allocated;

        throw std::bad_alloc{};
    }

    void deallocate(T* p, size_t n) noexcept
    {
        maybe_unused(n);
        supermalloc_free(p);
    }
};

#endif /* SUPERMALLOC_ALLOCATOR_H_ */

