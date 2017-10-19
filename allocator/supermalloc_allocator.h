#ifndef SUPERMALLOC_ALLOCATOR_H_
#define SUPERMALLOC_ALLOCATOR_H_

#include "supermalloc.h"
#include <new>
#include <type_traits>

// Neither gcc7 nor clang5 support std::pmr::memory_resource yet
// so we fallback on std::experimental::pmr or boost::container::pmr
// We put it in namespace supermalloc::pmr so the rest of the code
// doesn't have to care which version we are using
#if __has_include(<memory_resource>)
#include <memory_resource>
namespace supermalloc::pmr { using namespace std::pmr; }
#elif __has_include(<experimental/memory_resource>)
#include <experimental/memory_resource>
namespace supermalloc::pmr { using namespace std::experimental::pmr; }
#elif __has_include(<boost/container/pmr/memory_resource.hpp>) && \
      __has_include(<boost/container/pmr/polymorphic_allocator.hpp>) && \
      __has_include(<boost/container/pmr/pool_options.hpp>) && \
      __has_include(<boost/container/pmr/synchronized_pool_resource.hpp>) && \
      __has_include(<boost/container/pmr/unsynchronized_pool_resource.hpp>) && \
      __has_include(<boost/container/pmr/monotonic_buffer_resource.hpp>)
#include <boost/container/pmr/memory_resource.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <boost/container/pmr/pool_options.hpp>
#include <boost/container/pmr/synchronized_pool_resource.hpp>
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
namespace supermalloc::pmr { using namespace boost::container::pmr; }
#endif

namespace supermalloc
{
    template<typename... Ts> constexpr void maybe_unused(Ts&&...) noexcept {}

    template<typename T>
    class allocator
    {
    public:
        using value_type = T;
        using propagate_on_container_move_assignment = std::true_type;

        constexpr allocator() noexcept = default;

        template<typename U>
        constexpr allocator(allocator<U> const&) noexcept {}

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

    template<typename L, typename R>
    constexpr bool operator==(allocator<L> const&, allocator<R> const&) noexcept
    { return true; }

    template<typename L, typename R>
    constexpr bool operator!=(allocator<L> const& l, allocator<R> const& r) noexcept
    { return !(l == r); }

    class supermalloc_memory_resource : public pmr::memory_resource
    {
    protected:
        void* do_allocate(size_t bytes, size_t alignment) override
        { return supermalloc_aligned_alloc(alignment, bytes); }

        void do_deallocate(void* p, size_t bytes, size_t alignment) override
        { maybe_unused(bytes, alignment); supermalloc_free(p); }

        bool do_is_equal(const memory_resource& other) const noexcept override
        { maybe_unused(other); return true; }

    public:
        static supermalloc_memory_resource* mr() noexcept
        { static supermalloc_memory_resource smr; return &smr; }
    };

} // supermalloc namespace

#endif /* SUPERMALLOC_ALLOCATOR_H_ */

