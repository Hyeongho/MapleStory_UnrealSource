#pragma once

#include "TSharedPtr.h"

template<typename To, typename From>
TSharedPtr<To> StaticPointerCast(const TSharedPtr<From>& FromPtr)
{
    if (!FromPtr.IsValid())
    {
        return TSharedPtr<To>();
    }

    // TSharedPtr(Ptr, Counter)를 사용하여 참조 카운터 공유
    return TSharedPtr<To>(static_cast<To*>(FromPtr.Get()), FromPtr.Counter);
}