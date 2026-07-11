#pragma once
#include "Object/UClass.h"

template<typename T, typename U>
T* Cast(U* Obj)
{
    if (Obj && Obj->GetClass()->IsChildOf(T::StaticClass()))
    {
        return static_cast<T*>(Obj);
    }

    return nullptr;
}

template<typename T, typename U>
const T* Cast(const U* Obj)
{
    if (Obj && Obj->GetClass()->IsChildOf(T::StaticClass()))
    {
        return static_cast<const T*>(Obj);
    }

    return nullptr;
}

template<typename T, typename U>
T* CastChecked(U* Obj)
{
    T* Result = Cast<T>(Obj);
    check(Result != nullptr);
    return Result;
}

template<typename T, typename U>
const T* CastChecked(const U* Obj)
{
    const T* Result = Cast<T>(Obj);
    check(Result != nullptr);
    return Result;
}

template<typename T, typename U>
T* ExactCast(U* Obj)
{
    if (Obj && Obj->GetClass()->IsExactClass(T::StaticClass()))
    {
        return static_cast<T*>(Obj);
    }

    return nullptr;
}

template<typename T, typename U>
const T* ExactCast(const U* Obj)
{
    if (Obj && Obj->GetClass()->IsExactClass(T::StaticClass()))
    {
        return static_cast<const T*>(Obj);
    }

    return nullptr;
}