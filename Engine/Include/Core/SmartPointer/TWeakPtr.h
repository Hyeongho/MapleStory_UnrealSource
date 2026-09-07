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
            m_pRefCountBlock->AddWeak();
        }
    }

    TWeakPtr(const TWeakPtr& Other) noexcept : m_pElement(Other.m_pElement), m_pRefCountBlock(Other.m_pRefCountBlock)
    {
        if (m_pRefCountBlock)
        {
            m_pRefCountBlock->AddWeak();
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
            m_pRefCountBlock->AddWeak();
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
                m_pRefCountBlock->AddWeak();
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
        return m_pRefCountBlock != nullptr && m_pRefCountBlock->GetSharedCount() > 0;
    }

    TSharedPtr<T> Pin() const
    {
        // IsValid() 확인과 AddShared() 증가를 따로 하지 않는다 — 그 사이에
        // 마지막 strong reference가 해제될 수 있는 TOCTOU 레이스를 막기 위해
        // "0이 아닐 때만 증가"를 ConditionallyAddShared() 하나의 원자적 연산으로
        // 수행한다.
        if (!m_pRefCountBlock || !m_pRefCountBlock->ConditionallyAddShared())
        {
            return TSharedPtr<T>();
        }

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

        m_pRefCountBlock->ReleaseWeak();
    }

    template<typename U> friend class TWeakPtr;
};