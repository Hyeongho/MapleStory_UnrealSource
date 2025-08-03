#pragma once

//
// Memory.h - �޸� ��ƿ��Ƽ �Լ� �����
//

#include <cstring>
#include <cstdlib>

inline void* MemZero(void* Ptr, size_t Size)
{
    return std::memset(Ptr, 0, Size);
}

inline void* MemCopy(void* Dest, const void* Src, size_t Size)
{
    return std::memcpy(Dest, Src, Size);
}

inline void* MemAlloc(size_t Size)
{
    return std::malloc(Size);
}

inline void MemFree(void* Ptr)
{
    std::free(Ptr);
}
