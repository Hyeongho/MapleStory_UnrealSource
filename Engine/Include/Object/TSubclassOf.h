#pragma once
#include "Object/UClass.h"

template<typename T>
class TSubclassOf
{
public:
    TSubclassOf() : m_pClass(nullptr) {}

    TSubclassOf(UClass* InClass) : m_pClass(nullptr)
    {
        if (InClass)
        {
            check(InClass->IsChildOf(T::StaticClass()));
            m_pClass = InClass;
        }
    }

    template<typename U>
    TSubclassOf(const TSubclassOf<U>& Other) : m_pClass(Other.Get())
    {
        if (m_pClass)
            check(m_pClass->IsChildOf(T::StaticClass()));
    }

    UClass* Get()    const { return m_pClass; }
    bool IsValid()   const { return m_pClass != nullptr; }
    explicit operator bool() const { return IsValid(); }
    UClass* operator->() const { check(m_pClass); return m_pClass; }

    bool operator==(const TSubclassOf& Other) const { return m_pClass == Other.m_pClass; }
    bool operator!=(const TSubclassOf& Other) const { return m_pClass != Other.m_pClass; }

private:
    UClass* m_pClass;
};
