#pragma once
#include "TSharedPtr.h"

template<typename T>
class TWeakPtr
{
public:
    TWeakPtr() noexcept : m_pElement(nullptr), m_pRefCountBlock(nullptr)
    {
    }

    TWeakPtr(const TSharedPtr<T>& InSharedPtr) noexcept : m_pElement(InSharedPtr.m_pElement), m_pRefCountBlock(InSharedPtr.m_pRefCountBlock)
    {
        if (m_pRefCountBlock)
        {
            ++m_pRefCountBlock->m_WeakCount;
        }
    }

    TWeakPtr(const TWeakPtr& Other) noexcept : m_pElement(Other.m_pElement), m_pRefCountBlock(Other.m_pRefCountBlock)
    {
        if (m_pRefCountBlock)
        {
            ++m_pRefCountBlock->m_WeakCount;
        }
    }

    TWeakPtr(TWeakPtr&& Other) noexcept : m_pElement(Other.m_pElement), m_pRefCountBlock(Other.m_pRefCountBlock)
    {
        Other.m_pElement = nullptr;
        Other.m_pRefCountBlock = nullptr;
    }

    ~TWeakPtr()
    {
        ReleaseRef();
    }

    TWeakPtr& operator=(const TSharedPtr<T>& InSharedPtr) noexcept
    {
        ReleaseRef();

        m_pElement = InSharedPtr.m_pElement;
        m_pRefCountBlock = InSharedPtr.m_pRefCountBlock;

        if (m_pRefCountBlock)
        {
            ++m_pRefCountBlock->m_WeakCount;
        }

        return *this;
    }

    TWeakPtr& operator=(const TWeakPtr& Other) noexcept
    {
        if (this != &Other)
        {
            ReleaseRef();

            m_pElement = Other.m_pElement;
            m_pRefCountBlock = Other.m_pRefCountBlock;

            if (m_pRefCountBlock)
            {
                ++m_pRefCountBlock->m_WeakCount;
            }
        }

        return *this;
    }

    TWeakPtr& operator=(TWeakPtr&& Other) noexcept
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

    bool IsValid() const
    {
        return m_pRefCountBlock != nullptr && m_pRefCountBlock->m_SharedCount > 0;
    }

    TSharedPtr<T> Pin() const
    {
        if (!IsValid())
        {
            return TSharedPtr<T>();
        }

        ++m_pRefCountBlock->m_SharedCount;
        return TSharedPtr<T>(m_pElement, m_pRefCountBlock);
    }

    void Reset()
    {
        ReleaseRef();
        m_pElement = nullptr;
        m_pRefCountBlock = nullptr;
    }

private:
    T* m_pElement;
    FRefCountBlock* m_pRefCountBlock;

    void ReleaseRef()
    {
        if (!m_pRefCountBlock)
        {
            return;
        }

        if (--m_pRefCountBlock->m_WeakCount == 0 && m_pRefCountBlock->m_SharedCount == 0)
        {
            FMemory::Free(m_pRefCountBlock);
        }
    }

    template<typename U> friend class TWeakPtr;
};