#ifndef SUPERMALLOC_ALLOCATOR_H_
#define SUPERMALLOC_ALLOCATOR_H_

#include "supermalloc.h"
#include <new>
#include <type_traits>

// Neither gcc7 nor clang5 support std::pmr::memory_resource
// // so use std::experimental::pmr::memory_resource
// // and dump it in namespace pmr for now
#if __has_include(<memory_resource>)
#include <memory_resource>
namespace pmr { using namespace std::pmr; }
#elif __has_include(<experimental/memory_resource>)
#include <experimental/memory_resource>
namespace pmr { using namespace std::experimental::pmr; }
#endif

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

class supermalloc_memory_resource : public pmr::memory_resource
{
    template<typename... Ts> constexpr static void maybe_unused(Ts&&...) noexcept {}

protected:
    void* do_allocate(size_t bytes, size_t alignment) override
    { return supermalloc_aligned_alloc(alignment, bytes); }

    void do_deallocate(void* p, size_t bytes, size_t alignment) override
    { maybe_unused(bytes, alignment); supermalloc_free(p); }

    bool do_is_equal(const memory_resource& other) const noexcept override
    { maybe_unused(other); return true; }
};

#endif /* SUPERMALLOC_ALLOCATOR_H_ */

