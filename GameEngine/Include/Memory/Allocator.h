#pragma once

//
// Allocator.h - 기본 메모리 할당기 인터페이스
//

#include <cstdlib>
#include <new>

template<typename T>
class TDefaultAllocator
{
public:
    static T* Allocate(size_t Count = 1)
    {
        return static_cast<T*>(std::malloc(sizeof(T) * Count));
    }

    static void Deallocate(T* Ptr)
    {
        std::free(Ptr);
    }

    static void Construct(T* Ptr, const T& Value)
    {
        new (Ptr) T(Value);
    }

    static void Destruct(T* Ptr)
    {
        Ptr->~T();
    }
};
