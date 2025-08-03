#pragma once

#include "TSharedPtr.h"

template<typename To, typename From>
TSharedPtr<To> StaticPointerCast(const TSharedPtr<From>& FromPtr)
{
    if (!FromPtr.IsValid())
    {
        return TSharedPtr<To>();
    }

    // TSharedPtr(Ptr, Counter)�� ����Ͽ� ���� ī���� ����
    return TSharedPtr<To>(static_cast<To*>(FromPtr.Get()), FromPtr.Counter);
}