#pragma once

#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

using FSmartPtrDeleter = void(*)(void*);

struct FSmartPtrAtomics
{
    static int32 Increment(volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return (int32)_InterlockedIncrement(reinterpret_cast<volatile long*>(pValue));
#else
        return __atomic_add_fetch(pValue, 1, __ATOMIC_SEQ_CST);
#endif
    }

    static int32 Decrement(volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return (int32)_InterlockedDecrement(reinterpret_cast<volatile long*>(pValue));
#else
        return __atomic_sub_fetch(pValue, 1, __ATOMIC_SEQ_CST);
#endif
    }

    static int32 Load(const volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return *pValue;
#else
        return __atomic_load_n(pValue, __ATOMIC_SEQ_CST);
#endif
    }

    // *pDest == Comparand이면 *pDest = Exchange로 교체. 성공/실패와 무관하게
    // 호출 시점의 *pDest 원래 값을 반환한다(성공 여부는 반환값 == Comparand로 판별).
    static int32 CompareExchange(volatile int32* pDest, int32 Exchange, int32 Comparand)
    {
#if defined(_MSC_VER)
        return (int32)_InterlockedCompareExchange(reinterpret_cast<volatile long*>(pDest), Exchange, Comparand);
#else
        __atomic_compare_exchange_n(pDest, &Comparand, Exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        return Comparand;
#endif
    }
};

struct FRefCountBlock
{
    volatile int32 m_SharedCount;
    volatile int32 m_WeakCount;
    FSmartPtrDeleter m_Deleter;

    FRefCountBlock(FSmartPtrDeleter InDeleter) : m_SharedCount(1), m_WeakCount(1), m_Deleter(InDeleter)
    {
    }

    void AddShared()
    {
        FSmartPtrAtomics::Increment(&m_SharedCount);
    }

    // 0이 아닐 때만 원자적으로 증가시킨다 — TWeakPtr::Pin()의 "IsValid() 확인 후
    // AddShared()" 두 단계가 원자적으로 묶여있지 않으면, 그 사이에 다른 스레드가
    // 마지막 strong reference를 해제해 객체를 파괴시켰을 때 카운트를 0에서 1로
    // 되살리는(resurrection) use-after-free가 될 수 있다. compare-exchange 루프로
    // "현재 값이 0이 아니면 +1"을 단일 원자적 연산처럼 만든다. 성공(=0이 아니었음)하면
    // true를 반환.
    bool ConditionallyAddShared()
    {
        int32 Current = FSmartPtrAtomics::Load(&m_SharedCount);
        while (Current != 0)
        {
            const int32 Prev = FSmartPtrAtomics::CompareExchange(&m_SharedCount, Current + 1, Current);
            if (Prev == Current)
            {
                return true;
            }
            Current = Prev;
        }
        return false;
    }

    void AddWeak()
    {
        FSmartPtrAtomics::Increment(&m_WeakCount);
    }

    int32 GetSharedCount() const
    {
        return FSmartPtrAtomics::Load(&m_SharedCount);
    }

    void ReleaseShared(void* pElement)
    {
        if (FSmartPtrAtomics::Decrement(&m_SharedCount) == 0)
        {
            if (m_Deleter)
            {
                m_Deleter(pElement);
            }

            if (FSmartPtrAtomics::Decrement(&m_WeakCount) == 0)
            {
                FMemory::Free(this);
            }
        }
    }

    void ReleaseWeak()
    {
        if (FSmartPtrAtomics::Decrement(&m_WeakCount) == 0)
        {
            FMemory::Free(this);
        }
    }
};