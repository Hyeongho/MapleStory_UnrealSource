#pragma once
#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

//=============================================================================
// TSparseArray<T>
//
// Dynamic array whose elements keep a STABLE int32 index for their entire
// lifetime. Removing an element leaves a hole that goes onto an intrusive
// free list and is reused by the next Add/Emplace - no tombstones, no
// shifting, contiguous memory.
//
// Backing storage for the hash containers (TMap/TSet, Unreal-style):
// the hash buckets store element indices, which stay valid across removals.
//=============================================================================

template<typename T>
class TSparseArray
{
private:
    struct FSlot
    {
        alignas(T) uint8 m_Storage[sizeof(T)];
        int32 m_NextFree;     // next free slot index; valid only when not allocated
        bool  m_bAllocated;
    };

    FSlot* m_pSlots;
    int32 m_Capacity;         // slots in the buffer
    int32 m_HighWater;        // slots ever used; indices >= HighWater are untouched
    int32 m_NumAllocated;     // live element count
    int32 m_FirstFree;        // free list head inside [0, HighWater), INDEX_NONE if empty

    T* GetPtr(int32 Index)
    {
        return reinterpret_cast<T*>(m_pSlots[Index].m_Storage);
    }

    const T* GetPtr(int32 Index) const
    {
        return reinterpret_cast<const T*>(m_pSlots[Index].m_Storage);
    }

    // Relocates every used slot into a larger buffer. Slot indices never change.
    void Grow(int32 NewCapacity)
    {
        FSlot* pNew = (FSlot*)FMemory::Malloc(sizeof(FSlot) * NewCapacity, alignof(FSlot));
        check(pNew != nullptr);

        for (int32 i = 0; i < m_HighWater; i++)
        {
            pNew[i].m_bAllocated = m_pSlots[i].m_bAllocated;
            pNew[i].m_NextFree = m_pSlots[i].m_NextFree;

            if (m_pSlots[i].m_bAllocated)
            {
                if constexpr (TIsTriviallyCopyable<T>::Value)
                {
                    FMemory::Memcpy(pNew[i].m_Storage, m_pSlots[i].m_Storage, sizeof(T));
                }

                else
                {
                    new (pNew[i].m_Storage) T(MoveTemp(*GetPtr(i)));
                    GetPtr(i)->~T();
                }
            }
        }

        if (m_pSlots)
        {
            FMemory::Free(m_pSlots);
        }

        m_pSlots = pNew;
        m_Capacity = NewCapacity;
    }

    // Structural copy: identical indices, high water and free list as Other.
    // Index stability across the copy is what lets TMap/TSet copy their
    // bucket arrays verbatim.
    void CopyFrom(const TSparseArray& Other)
    {
        if (Other.m_HighWater > 0)
        {
            Grow(Other.m_HighWater);

            for (int32 i = 0; i < Other.m_HighWater; i++)
            {
                m_pSlots[i].m_bAllocated = Other.m_pSlots[i].m_bAllocated;
                m_pSlots[i].m_NextFree = Other.m_pSlots[i].m_NextFree;

                if (Other.m_pSlots[i].m_bAllocated)
                {
                    new (m_pSlots[i].m_Storage) T(*Other.GetPtr(i));
                }
            }
        }

        m_HighWater = Other.m_HighWater;
        m_NumAllocated = Other.m_NumAllocated;
        m_FirstFree = Other.m_FirstFree;
    }

public:
    TSparseArray() : m_pSlots(nullptr), m_Capacity(0), m_HighWater(0), m_NumAllocated(0), m_FirstFree(INDEX_NONE)
    {
    }

    TSparseArray(const TSparseArray& Other) : m_pSlots(nullptr), m_Capacity(0), m_HighWater(0), m_NumAllocated(0), m_FirstFree(INDEX_NONE)
    {
        CopyFrom(Other);
    }

    TSparseArray(TSparseArray&& Other) noexcept
        : m_pSlots(Other.m_pSlots), m_Capacity(Other.m_Capacity), m_HighWater(Other.m_HighWater), m_NumAllocated(Other.m_NumAllocated), m_FirstFree(Other.m_FirstFree)
    {
        Other.m_pSlots = nullptr;
        Other.m_Capacity = 0;
        Other.m_HighWater = 0;
        Other.m_NumAllocated = 0;
        Other.m_FirstFree = INDEX_NONE;
    }

    ~TSparseArray()
    {
        Empty();
    }

    TSparseArray& operator=(const TSparseArray& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        Empty();
        CopyFrom(Other);

        return *this;
    }

    TSparseArray& operator=(TSparseArray&& Other) noexcept
    {
        if (this == &Other)
        {
            return *this;
        }

        Empty();

        m_pSlots = Other.m_pSlots;
        m_Capacity = Other.m_Capacity;
        m_HighWater = Other.m_HighWater;
        m_NumAllocated = Other.m_NumAllocated;
        m_FirstFree = Other.m_FirstFree;

        Other.m_pSlots = nullptr;
        Other.m_Capacity = 0;
        Other.m_HighWater = 0;
        Other.m_NumAllocated = 0;
        Other.m_FirstFree = INDEX_NONE;

        return *this;
    }

    // -------------------------------------------------------------------------
    // Insert - reuses a free slot when available, appends otherwise
    // -------------------------------------------------------------------------

    template<typename... Args>
    int32 Emplace(Args&&... InArgs)
    {
        int32 Index;

        if (m_FirstFree != INDEX_NONE)
        {
            Index = m_FirstFree;
            m_FirstFree = m_pSlots[Index].m_NextFree;
        }

        else
        {
            if (m_HighWater == m_Capacity)
            {
                Grow(m_Capacity == 0 ? 4 : m_Capacity * 2);
            }

            Index = m_HighWater++;
        }

        new (m_pSlots[Index].m_Storage) T(Forward<Args>(InArgs)...);
        m_pSlots[Index].m_bAllocated = true;
        m_NumAllocated++;

        return Index;
    }

    int32 Add(const T& Element)
    {
        return Emplace(Element);
    }

    int32 Add(T&& Element)
    {
        return Emplace(MoveTemp(Element));
    }

    // -------------------------------------------------------------------------
    // Remove - O(1), the slot goes onto the free list
    // -------------------------------------------------------------------------

    void RemoveAt(int32 Index)
    {
        check(IsAllocated(Index));

        GetPtr(Index)->~T();

        m_pSlots[Index].m_bAllocated = false;
        m_pSlots[Index].m_NextFree = m_FirstFree;
        m_FirstFree = Index;
        m_NumAllocated--;
    }

    // -------------------------------------------------------------------------
    // Access
    // -------------------------------------------------------------------------

    bool IsAllocated(int32 Index) const
    {
        return Index >= 0 && Index < m_HighWater && m_pSlots[Index].m_bAllocated;
    }

    T& operator[](int32 Index)
    {
        check(IsAllocated(Index));
        return *GetPtr(Index);
    }

    const T& operator[](int32 Index) const
    {
        check(IsAllocated(Index));
        return *GetPtr(Index);
    }

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    int32 Num() const
    {
        return m_NumAllocated;
    }

    // One past the highest index ever used; iterate [0, GetMaxIndex()) with IsAllocated
    int32 GetMaxIndex() const
    {
        return m_HighWater;
    }

    bool IsEmpty() const
    {
        return m_NumAllocated == 0;
    }

    // -------------------------------------------------------------------------
    // Memory
    // -------------------------------------------------------------------------

    // Destroy all elements, keep the buffer
    void Reset()
    {
        for (int32 i = 0; i < m_HighWater; i++)
        {
            if (m_pSlots[i].m_bAllocated)
            {
                GetPtr(i)->~T();
                m_pSlots[i].m_bAllocated = false;
            }
        }

        m_HighWater = 0;
        m_NumAllocated = 0;
        m_FirstFree = INDEX_NONE;
    }

    // Destroy all elements and free the buffer
    void Empty()
    {
        Reset();

        if (m_pSlots)
        {
            FMemory::Free(m_pSlots);
            m_pSlots = nullptr;
        }

        m_Capacity = 0;
    }

    // -------------------------------------------------------------------------
    // Iterator - skips holes
    // -------------------------------------------------------------------------

    struct FIterator
    {
        TSparseArray* m_pArray;
        int32 m_Index;

        FIterator(TSparseArray* pArray, int32 Index) : m_pArray(pArray), m_Index(Index)
        {
            SkipToAllocated();
        }

        void SkipToAllocated()
        {
            while (m_Index < m_pArray->m_HighWater && !m_pArray->m_pSlots[m_Index].m_bAllocated)
            {
                m_Index++;
            }
        }

        FIterator& operator++()
        {
            m_Index++;
            SkipToAllocated();
            return *this;
        }

        bool operator!=(const FIterator& Other) const
        {
            return m_Index != Other.m_Index;
        }

        T& operator*() const
        {
            return (*m_pArray)[m_Index];
        }

        T* operator->() const
        {
            return &(*m_pArray)[m_Index];
        }

        int32 GetIndex() const
        {
            return m_Index;
        }
    };

    struct FConstIterator
    {
        const TSparseArray* m_pArray;
        int32 m_Index;

        FConstIterator(const TSparseArray* pArray, int32 Index) : m_pArray(pArray), m_Index(Index)
        {
            SkipToAllocated();
        }

        void SkipToAllocated()
        {
            while (m_Index < m_pArray->m_HighWater && !m_pArray->m_pSlots[m_Index].m_bAllocated)
            {
                m_Index++;
            }
        }

        FConstIterator& operator++()
        {
            m_Index++;
            SkipToAllocated();
            return *this;
        }

        bool operator!=(const FConstIterator& Other) const
        {
            return m_Index != Other.m_Index;
        }

        const T& operator*() const
        {
            return (*m_pArray)[m_Index];
        }

        const T* operator->() const
        {
            return &(*m_pArray)[m_Index];
        }

        int32 GetIndex() const
        {
            return m_Index;
        }
    };

    FIterator begin()
    {
        return FIterator(this, 0);
    }

    FIterator end()
    {
        return FIterator(this, m_HighWater);
    }

    FConstIterator begin() const
    {
        return FConstIterator(this, 0);
    }

    FConstIterator end() const
    {
        return FConstIterator(this, m_HighWater);
    }
};
