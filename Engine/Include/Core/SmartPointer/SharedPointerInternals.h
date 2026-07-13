#pragma once

#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

using FSmartPtrDeleter = void(*)(void*);

// Atomic reference-count operations. Compiler-branched so the shared-pointer
// reference counting is thread-safe on MSVC (Interlocked intrinsics) while
// still building and running under GCC/Clang for verification.
struct FSmartPtrAtomics
{
    // Returns the value AFTER the increment.
    static int32 Increment(volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return (int32)_InterlockedIncrement(reinterpret_cast<volatile long*>(pValue));
#else
        return __atomic_add_fetch(pValue, 1, __ATOMIC_SEQ_CST);
#endif
    }

    // Returns the value AFTER the decrement.
    static int32 Decrement(volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return (int32)_InterlockedDecrement(reinterpret_cast<volatile long*>(pValue));
#else
        return __atomic_sub_fetch(pValue, 1, __ATOMIC_SEQ_CST);
#endif
    }

    // Plain atomic load for read-only queries (GetRefCount / IsValid).
    static int32 Load(const volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return *pValue;
#else
        return __atomic_load_n(pValue, __ATOMIC_SEQ_CST);
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

    void AddWeak()
    {
        FSmartPtrAtomics::Increment(&m_WeakCount);
    }

    int32 GetSharedCount() const
    {
        return FSmartPtrAtomics::Load(&m_SharedCount);
    }

    // Releases one shared reference. If it was the last shared owner, runs the
    // deleter and then releases the implicit weak reference held by the shared
    // group. Frees the block when no weak references remain.
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

    // Releases one weak reference and frees the block when none remain.
    void ReleaseWeak()
    {
        if (FSmartPtrAtomics::Decrement(&m_WeakCount) == 0)
        {
            FMemory::Free(this);
        }
    }
};