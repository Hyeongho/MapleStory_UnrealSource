#pragma once

#include "ICastable.h"

#include "../SmartPointer/TSharedPtr.h"

#include <type_traits>
#include <cassert>

template<typename To, typename From>
To* StaticCast(From* obj)
{
    return static_cast<To*>(obj);
}

template<typename To, typename From>
To* DynamicCast(From* obj)
{
    static_assert(std::is_base_of<ICastable, From>::value, "From must inherit from ICastable");
    static_assert(std::is_base_of<ICastable, To>::value, "To must inherit from ICastable");

    if (!obj)
    {
        return nullptr;
    }

    if (obj->GetTypeID() == To::StaticTypeID())
    {
        return static_cast<To*>(obj);
    }

    return nullptr;
}

template<typename To, typename From>
TSharedPtr<To> StaticPointerCast(const TSharedPtr<From>& obj)
{
    To* casted = static_cast<To*>(obj.Get());
    return TSharedPtr<To>(obj, static_cast<To*>(obj.Get()));
}

template<typename To, typename From>
TSharedPtr<To> DynamicPointerCast(const TSharedPtr<From>& obj)
{
    static_assert(std::is_base_of<ICastable, From>::value, "From must inherit from ICastable");
    static_assert(std::is_base_of<ICastable, To>::value, "To must inherit from ICastable");

    if (!obj || obj->Get() == nullptr)
    {
        return nullptr;
    }

    if (obj->Get()->GetTypeID() == To::StaticTypeID())
    {
        return TSharedPtr<To>(obj, static_cast<To*>(obj.Get()));
    }

    return nullptr;
}

template<typename To, typename From>
To* CastChecked(From* obj)
{
    To* casted = DynamicCast<To>(obj);
    assert(casted && "CastChecked failed: type mismatch or nullptr");
    return casted;
}
