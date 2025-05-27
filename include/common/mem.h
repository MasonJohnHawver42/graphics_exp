#ifndef _MEM_H
#define _MEM_H

#include "log.h"

#include <memory>
#include <memory_resource>

#include <type_traits>
#include <utility>
#include <new>

#ifdef DEBUG
#include <cpptrace/cpptrace.hpp>
#include <iostream>
#endif

namespace cm 
{

    template <typename T>
    struct pmr_deleter 
    {
        std::pmr::memory_resource* resource;

        void operator()(T* ptr) const 
        {
            if (ptr) 
            {
                ptr->~T();
                resource->deallocate(ptr, sizeof(T), alignof(T));
            }
        }
    };

    template <typename T>
    using unique_ptr = std::unique_ptr<T, pmr_deleter<T>>;

    template <typename T, typename... Args>
    unique_ptr<T> allocate_unique(std::pmr::memory_resource* resource, Args&&... args) 
    {
        void* mem = resource->allocate(sizeof(T), alignof(T));
        T* obj = new (mem) T(std::forward<Args>(args)...);
        return std::unique_ptr<T, pmr_deleter<T>>(obj, pmr_deleter<T>{resource});
    }

    template <typename B, typename D, typename... Args, typename = std::enable_if_t<std::is_base_of_v<B, D>>>
    unique_ptr<B> allocate_unique(std::pmr::memory_resource* resource, Args&&... args) 
    {
        void* mem = resource->allocate(sizeof(D), alignof(D));
        B* obj = new (mem) D(std::forward<Args>(args)...);
        return std::unique_ptr<B, pmr_deleter<B>>(obj, pmr_deleter<B>{resource});
    }

    template <typename T>
    struct Manual {
        Manual() = default;

        Manual(const Manual&) = delete;
        Manual& operator=(const Manual&) = delete;

        #ifdef DEBUG
        ~Manual() noexcept(false) {
            ASSERT(constructed, "Manual<T>::destroy() called before construct()");
            get()->~T();
        }
        #else
        ~Manual()  {
            get()->~T();
        }
        #endif

        template <typename... Args>
        T* construct(Args&&... args) {
        #ifdef DEBUG
            ASSERT(!constructed, "Manual<T> constructed twice!");
            constructed = true;
            #endif
            return new (&storage) T(std::forward<Args>(args)...);
        }

        void destroy() {
            #ifdef DEBUG
            ASSERT(constructed, "Manual<T>::destroy() called before construct()");
            constructed = false;
            #endif
            get()->~T();
        }

        T* get() { return std::launder(reinterpret_cast<T*>(&storage)); }
        T& operator*() { return *get(); }
        T* operator->() { return get(); }

    private:
        alignas(T) std::byte storage[sizeof(T)];

        #ifdef DEBUG
        bool constructed = false;
        #endif
    };

}


#endif // _MEM_H