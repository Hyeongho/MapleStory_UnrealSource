#pragma once

#include "TSharedPtr.h"
#include "TWeakPtr.h"

template<typename T>
inline bool IsValidPtr(T* Ptr)
{
    return Ptr != nullptr;
}

template<typename T>
inline bool IsValidPtr(const TSharedPtr<T>& Ptr)
{
    return Ptr.IsValid();
}

template<typename T>
inline bool IsValidPtr(const TWeakPtr<T>& Ptr)
{
    return Ptr.IsValid();
}
