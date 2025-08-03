#pragma once

#include "TSharedPtr.h"
#include "TWeakPtr.h"


template<typename T>
inline bool TIsValid(T* Ptr)
{
    return Ptr != nullptr;
}

template<typename T>
inline bool TIsValid(const TSharedPtr<T>& Ptr)
{
    return Ptr.IsValid();
}

template<typename T>
inline bool TIsValid(const TWeakPtr<T>& Ptr)
{
    return Ptr.IsValid();
}
