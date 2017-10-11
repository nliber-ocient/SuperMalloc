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
namespace supermalloc::pmr { using namespace std::pmr; }
#elif __has_include(<experimental/memory_resource>)
#include <experimental/memory_resource>
namespace supermalloc::pmr { using namespace std::experimental::pmr; }
#elif __has_include(<boost/container/pmr/memory_resource.hpp>)
#include <boost/container/pmr/memory_resource.hpp>
namespace supermalloc::pmr { using namespace boost::container::pmr; }
#else
#include <cstddef>

namespace supermalloc::pmr
{
    class memory_resource
    {
    public:
        virtual ~memory_resource() = default;

        void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t))
        { return do_allocate(bytes, alignment); }

        void deallocate(void* p, size_t bytes, size_t alignment = alignof(std::max_align_t))
        { return do_deallocate(p, bytes, alignment); }

        bool is_equal(const memory_resource& other) const noexcept
        { return do_is_equal(other); }

        friend bool operator==(const memory_resource& a, const memory_resource& b) noexcept
        { return &a == &b || a.is_equal(b); }

        friend bool operator!=(const memory_resource& a, const memory_resource& b) noexcept
        { return !(a==b); }

    private:
        virtual void* do_allocate(size_t bytes, size_t alignment) = 0;
        virtual void do_deallocate(void* p, size_t bytes, size_t alignment) = 0;

        virtual bool do_is_equal(const memory_resource& other) const noexcept = 0;
    };
} // supermalloc::pmr namespace

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
        using is_always_equal = std::true_type;

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

