#pragma once

#include "TSharedPtr.h"

template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... args)
{
    return TSharedPtr<T>(new T(std::forward<Args>(args)...));
}