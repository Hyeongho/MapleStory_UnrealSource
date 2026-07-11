#pragma once
#include "SharedPointerInternals.h"
#include "Core/Templates/Utility.h"

template<typename T>
class TWeakPtr;

template<typename T>
class TSharedPtr
{
public:
    TSharedPtr() noexcept: m_pElement(nullptr), m_pRefCountBlock(nullptr)
    {
    }

    explicit TSharedPtr(T* InPtr) : m_pElement(InPtr), m_pRefCountBlock(nullptr)
    {
        if (InPtr)
        {
            m_pRefCountBlock = static_cast<FRefCountBlock*>(FMemory::Malloc(sizeof(FRefCountBlock)));

            new (m_pRefCountBlock) FRefCountBlock(&TSharedPtr::DefaultDeleter);
        }
    }

    TSharedPtr(const TSharedPtr& Other) noexcept : m_pElement(Other.m_pElement), m_pRefCountBlock(Other.m_pRefCountBlock)
    {
        if (m_pRefCountBlock)
        {
            m_pRefCountBlock->m_SharedCount++;
        }
    }

    TSharedPtr(TSharedPtr&& Other) noexcept : m_pElement(Other.m_pElement), m_pRefCountBlock(Other.m_pRefCountBlock)
    {
        Other.m_pElement = nullptr;
        Other.m_pRefCountBlock = nullptr;
    }

    template<typename U>
    TSharedPtr(const TSharedPtr<U>& Other) noexcept : m_pElement(static_cast<T*>(Other.m_pElement)), m_pRefCountBlock(Other.m_pRefCountBlock)
    {
        if (m_pRefCountBlock)
        {
            m_pRefCountBlock->m_SharedCount++;
        }
    }

    TSharedPtr& operator=(const TSharedPtr& Other) noexcept
    {
        if (this != &Other)
        {
            ReleaseRef();
            m_pElement = Other.m_pElement;
            m_pRefCountBlock = Other.m_pRefCountBlock;

            if (m_pRefCountBlock)
            {
                m_pRefCountBlock->m_SharedCount++;
            }
        }

        return *this;
    }

    TSharedPtr& operator=(TSharedPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            ReleaseRef();
            m_pElement = Other.m_pElement;
            m_pRefCountBlock = Other.m_pRefCountBlock;
            Other.m_pElement = nullptr;
            Other.m_pRefCountBlock = nullptr;
        }

        return *this;
    }

    ~TSharedPtr() 
    { 
        ReleaseRef(); 
    }

    T* operator->() const 
    { 
        check(m_pElement != nullptr); return m_pElement; 
    }

    T& operator*() const 
    { 
        check(m_pElement != nullptr); return *m_pElement; 
    }

    T* Get() const 
    { 
        return m_pElement; 
    }

    bool IsValid() const 
    { 
        return m_pElement != nullptr; 
    }

    int32 GetRefCount() const 
    { 
        return m_pRefCountBlock ? m_pRefCountBlock->m_SharedCount : 0; 
    }

    explicit operator bool() const 
    { 
        return IsValid(); 
    }

    void Reset()
    {
        ReleaseRef();
        m_pElement = nullptr;
        m_pRefCountBlock = nullptr;
    }

    bool operator==(const TSharedPtr& Other) const 
    { 
        return m_pElement == Other.m_pElement;
    }

    bool operator!=(const TSharedPtr& Other) const 
    { 
        return m_pElement != Other.m_pElement;
    }

private:
    T* m_pElement;
    FRefCountBlock* m_pRefCountBlock;

    TSharedPtr(T* InPtr, FRefCountBlock* InBlock) noexcept : m_pElement(InPtr), m_pRefCountBlock(InBlock)
    {
    }

    void ReleaseRef()
    {
        if (!m_pRefCountBlock)
        {
            return;
        }

        m_pRefCountBlock->m_SharedCount--;

        if (m_pRefCountBlock->m_SharedCount == 0)
        {
            if (m_pRefCountBlock->m_Deleter)
            {
                m_pRefCountBlock->m_Deleter(m_pElement);
            }

            m_pRefCountBlock->m_WeakCount--;

            if (m_pRefCountBlock->m_WeakCount == 0)
            {
                FMemory::Free(m_pRefCountBlock);
            }
        }
    }

    static void DefaultDeleter(void* p)
    {
        static_cast<T*>(p)->~T();
        FMemory::Free(p);
    }

    template<typename U> friend class TWeakPtr;
    template<typename U> friend class TSharedPtr;
};

template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... InArgs)
{
    T* Ptr = static_cast<T*>(FMemory::Malloc(sizeof(T), alignof(T)));
    new (Ptr) T(Forward<Args>(InArgs)...);
    return TSharedPtr<T>(Ptr);
}