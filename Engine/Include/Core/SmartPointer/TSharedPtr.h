#pragma once
#include "SharedPointerInternals.h"
#include "Core/Templates/Utility.h"

template<typename T>
class TWeakPtr;

template<typename T>
class TSharedPtr
{
public:
    TSharedPtr() noexcept : m_pElement(nullptr), m_pRefCountBlock(nullptr)
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
            m_pRefCountBlock->AddShared();
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
            m_pRefCountBlock->AddShared();
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
                m_pRefCountBlock->AddShared();
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
        return m_pRefCountBlock ? m_pRefCountBlock->GetSharedCount() : 0;
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

        m_pRefCountBlock->ReleaseShared(m_pElement);
    }

    static void DefaultDeleter(void* p)
    {
        static_cast<T*>(p)->~T();
        FMemory::Free(p);
    }

    // MakeShared<T>() 전용 디폴터 — 객체가 컨트롤 블록(FInlineRefCountBlock<T>)
    // 안에 인라인으로 저장되어 있으므로 소멸자만 호출하고 메모리는 해제하지
    // 않는다. 실제 힙 해제는 FRefCountBlock::ReleaseShared()가 WeakCount 0일 때
    // 수행하는 FMemory::Free(this) 한 번이 통합 블록 전체(컨트롤 블록+객체)를
    // 커버한다.
    static void InlineDeleter(void* p)
    {
        static_cast<T*>(p)->~T();
    }

    template<typename U> friend class TWeakPtr;
    template<typename U> friend class TSharedPtr;
    template<typename U, typename... Args> friend TSharedPtr<U> MakeShared(Args&&... InArgs);
};

// MakeShared<T>()가 객체를 컨트롤 블록 안에 인라인으로 저장해 힙 할당을
// (객체 1회 + 컨트롤 블록 1회 = 2회가 아니라) 1회로 합치기 위한 통합 블록.
// FRefCountBlock을 첫 번째 non-virtual 베이스로 단일 상속하므로, 이 구조체의
// 시작 주소와 FRefCountBlock*로 본 주소가 같다 — ReleaseShared()가 WeakCount 0일
// 때 FMemory::Free(this)로 이 블록 전체(따라서 그 안의 객체 스토리지까지)를
// 한 번에 해제할 수 있는 이유.
template<typename T>
struct FInlineRefCountBlock : public FRefCountBlock
{
    alignas(T) uint8 m_ObjectStorage[sizeof(T)];

    FInlineRefCountBlock(FSmartPtrDeleter InDeleter) : FRefCountBlock(InDeleter)
    {
    }

    T* GetObject()
    {
        return reinterpret_cast<T*>(m_ObjectStorage);
    }
};

template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... InArgs)
{
    using FBlock = FInlineRefCountBlock<T>;

    FBlock* pBlock = static_cast<FBlock*>(FMemory::Malloc(sizeof(FBlock), alignof(FBlock)));
    new (pBlock) FBlock(&TSharedPtr<T>::InlineDeleter);

    T* pObject = pBlock->GetObject();
    new (pObject) T(Forward<Args>(InArgs)...);

    return TSharedPtr<T>(pObject, pBlock);
}