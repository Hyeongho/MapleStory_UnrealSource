#pragma once
#include "TWeakPtr.h"

template<typename T>
class TSharedRef
{
public:
    explicit TSharedRef(T* InPtr) : m_SharedPtr(InPtr)
    {
        check(InPtr != nullptr);
    }

    TSharedRef(const TSharedRef& Other) noexcept : m_SharedPtr(Other.m_SharedPtr)
    {
    }

    TSharedRef(TSharedRef&& Other) noexcept : m_SharedPtr(MoveTemp(Other.m_SharedPtr))
    {
    }

    TSharedRef& operator=(const TSharedRef& Other) noexcept
    {
        m_SharedPtr = Other.m_SharedPtr;
        return *this;
    }

    TSharedRef& operator=(TSharedRef&& Other) noexcept
    {
        m_SharedPtr = MoveTemp(Other.m_SharedPtr);
        return *this;
    }

    T& operator*() const 
    { 
        return *m_SharedPtr; 
    }

    T* operator->() const 
    { 
        return m_SharedPtr.Get(); 
    }

    T* Get() const 
    { 
        return m_SharedPtr.Get(); 
    }

    int32 GetRefCount() const 
    {
        return m_SharedPtr.GetRefCount(); 
    }

    TSharedPtr<T> ToSharedPtr() const 
    { 
        return m_SharedPtr; 
    }

    bool operator==(const TSharedRef& Other) const 
    { 
        return m_SharedPtr == Other.m_SharedPtr; 
    }

    bool operator!=(const TSharedRef& Other) const 
    { 
        return m_SharedPtr != Other.m_SharedPtr; 
    }

private:
    TSharedPtr<T> m_SharedPtr;
};
