#pragma once
#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"
#include "HashFunctions.h"

template<typename KeyType>
class TSet
{
private:
    enum class EBucketState : uint8 
    { 
        Empty, 
        Occupied, 
        Deleted 
    };

    struct FBucket
    {
        KeyType Key;
        EBucketState State;

        FBucket() : Key(), State(EBucketState::Empty) 
        {

        }
    };

    FBucket* m_pBuckets;
    int32    m_Count;
    int32    m_Capacity;
    int32    m_DeletedCount;

    static const int32 INITIAL_CAPACITY = 16;

    int32 ProbeIndex(const KeyType& Key) const
    {
        uint32 Hash = GetTypeHash(Key);
        int32 Index = (int32)(Hash & (uint32)(m_Capacity - 1));
        int32 Start = Index;

        while(true)
        {
            if (m_pBuckets[Index].State == EBucketState::Empty)
            {
                return INDEX_NONE;
            }

            if (m_pBuckets[Index].State == EBucketState::Occupied && m_pBuckets[Index].Key == Key)
            {
                return Index;
            }

            Index = (Index + 1) & (m_Capacity - 1);

            if (Index == Start)
            {
                return INDEX_NONE;
            }
        }
    }

    int32 ProbeInsertIndex(const KeyType& Key) const
    {
        uint32 Hash = GetTypeHash(Key);
        int32 Index = (int32)(Hash & (uint32)(m_Capacity - 1));
        int32 FirstDeleted = INDEX_NONE;
        int32 Start = Index;

        while (true)
        {
            EBucketState State = m_pBuckets[Index].State;
            if (State == EBucketState::Empty)
            {
                return (FirstDeleted != INDEX_NONE) ? FirstDeleted : Index;
            }

            if (State == EBucketState::Deleted)
            {
                if (FirstDeleted == INDEX_NONE)
                {
                    FirstDeleted = Index;
                }
            }

            else if (m_pBuckets[Index].Key == Key)
            {
                return Index;
            }

            Index = (Index + 1) & (m_Capacity - 1);

            if (Index == Start)
            {
                return (FirstDeleted != INDEX_NONE) ? FirstDeleted : INDEX_NONE;
            }
        }
    }

    bool NeedsRehash() const
    {
        return (m_Count + m_DeletedCount + 1) * 4 > m_Capacity * 3;
    }

    void Rehash(int32 NewCapacity)
    {
        FBucket* OldBuckets = m_pBuckets;
        int32 OldCapacity = m_Capacity;

        m_pBuckets = (FBucket*)FMemory::Malloc(sizeof(FBucket) * NewCapacity, alignof(FBucket));

        for (int32 i = 0; i < NewCapacity; i++)
        {
            new (&m_pBuckets[i]) FBucket();
        }

        m_Capacity = NewCapacity;
        m_Count = 0;
        m_DeletedCount = 0;

        if (OldBuckets)
        {
            for (int32 i = 0; i < OldCapacity; i++)
            {
                if (OldBuckets[i].State == EBucketState::Occupied)
                {
                    Add(MoveTemp(OldBuckets[i].Key));
                }

                OldBuckets[i].~FBucket();
            }

            FMemory::Free(OldBuckets);
        }
    }

    void AllocateIfEmpty()
    {
        if (m_pBuckets == nullptr)
        {
            m_Capacity = INITIAL_CAPACITY;
            m_pBuckets = (FBucket*)FMemory::Malloc(sizeof(FBucket) * m_Capacity, alignof(FBucket));

            for (int32 i = 0; i < m_Capacity; i++)
            {
                new (&m_pBuckets[i]) FBucket();
            }
        }
    }

public:
    TSet() : m_pBuckets(nullptr), m_Count(0), m_Capacity(0), m_DeletedCount(0) 
    {

    }

    TSet(const TSet& Other) : m_pBuckets(nullptr), m_Count(0), m_Capacity(0), m_DeletedCount(0)
    {
        *this = Other;
    }

    TSet(TSet&& Other) noexcept : m_pBuckets(Other.m_pBuckets), m_Count(Other.m_Count), m_Capacity(Other.m_Capacity), m_DeletedCount(Other.m_DeletedCount)
    {
        Other.m_pBuckets = nullptr;
        Other.m_Count = 0;
        Other.m_Capacity = 0;
        Other.m_DeletedCount = 0;
    }

    ~TSet()
    {
        if (m_pBuckets)
        {
            for (int32 i = 0; i < m_Capacity; i++)
            {
                m_pBuckets[i].~FBucket();
            }

            FMemory::Free(m_pBuckets);

            m_pBuckets = nullptr;
        }
    }

    TSet& operator=(const TSet& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        Empty();

        for (int32 i = 0; i < Other.m_Capacity; i++)
        {
            if (Other.m_pBuckets[i].State == EBucketState::Occupied)
            {
                Add(Other.m_pBuckets[i].Key);
            }
        }

        return *this;
    }

    TSet& operator=(TSet&& Other) noexcept
    {
        if (this == &Other)
        {
            return *this;
        }

        if (m_pBuckets)
        {
            for (int32 i = 0; i < m_Capacity; i++)
            {
                m_pBuckets[i].~FBucket();
            }

            FMemory::Free(m_pBuckets);
        }

        m_pBuckets = Other.m_pBuckets;
        m_Count = Other.m_Count;
        m_Capacity = Other.m_Capacity;
        m_DeletedCount = Other.m_DeletedCount;

        Other.m_pBuckets = nullptr;
        Other.m_Count = 0;
        Other.m_Capacity = 0;
        Other.m_DeletedCount = 0;

        return *this;
    }

    // -------------------------------------------------------------------------
    // Insert
    // -------------------------------------------------------------------------

    bool Add(const KeyType& Key)
    {
        AllocateIfEmpty();

        if (NeedsRehash())
        {
            Rehash(m_Capacity * 2);
        }

        int32 Index = ProbeInsertIndex(Key);
        check(Index != INDEX_NONE);

        if (m_pBuckets[Index].State == EBucketState::Occupied)
        {
            return false; // already exists
        }

        if (m_pBuckets[Index].State == EBucketState::Deleted)
        {
            m_DeletedCount--;
        }

        m_pBuckets[Index].~FBucket();

        new (&m_pBuckets[Index]) FBucket();
        m_pBuckets[Index].Key = Key;
        m_pBuckets[Index].State = EBucketState::Occupied;

        m_Count++;

        return true;
    }

    bool Add(KeyType&& Key)
    {
        AllocateIfEmpty();

        if (NeedsRehash())
        {
            Rehash(m_Capacity * 2);
        }

        int32 Index = ProbeInsertIndex(Key);
        check(Index != INDEX_NONE);

        if (m_pBuckets[Index].State == EBucketState::Occupied)
        {
            return false; // already exists
        }

        if (m_pBuckets[Index].State == EBucketState::Deleted)
        {
            m_DeletedCount--;
        }

        m_pBuckets[Index].~FBucket();

        new (&m_pBuckets[Index]) FBucket();
        m_pBuckets[Index].Key = MoveTemp(Key);
        m_pBuckets[Index].State = EBucketState::Occupied;
        m_Count++;

        return true;
    }

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------

    bool Contains(const KeyType& Key) const
    {
        if (!m_pBuckets || m_Count == 0)
        {
            return false;
        }

        return ProbeIndex(Key) != INDEX_NONE;
    }

    // -------------------------------------------------------------------------
    // Remove
    // -------------------------------------------------------------------------

    bool Remove(const KeyType& Key)
    {
        if (!m_pBuckets || m_Count == 0)
        {
            return false;
        }

        int32 Index = ProbeIndex(Key);

        if (Index == INDEX_NONE)
        {
            return false;
        }

        m_pBuckets[Index].~FBucket();

        new (&m_pBuckets[Index]) FBucket();
        m_pBuckets[Index].State = EBucketState::Deleted;
        m_Count--;
        m_DeletedCount++;

        return true;
    }

    void Reset()
    {
        if (!m_pBuckets)
        {
            return;
        }

        for (int32 i = 0; i < m_Capacity; i++)
        {
            if (m_pBuckets[i].State != EBucketState::Empty)
            {
                m_pBuckets[i].~FBucket();
                new (&m_pBuckets[i]) FBucket();
            }
        }

        m_Count = 0;
        m_DeletedCount = 0;
    }

    void Empty()
    {
        if (m_pBuckets)
        {
            for (int32 i = 0; i < m_Capacity; i++)
            {
                m_pBuckets[i].~FBucket();
            }

            FMemory::Free(m_pBuckets);
            m_pBuckets = nullptr;
        }

        m_Count = 0;
        m_Capacity = 0;
        m_DeletedCount = 0;
    }

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    int32 Num() const 
    { 
        return m_Count; 
    }

    bool IsEmpty() const 
    { 
        return m_Count == 0; 
    }

    // -------------------------------------------------------------------------
    // Iterator
    // -------------------------------------------------------------------------

    struct FIterator
    {
        FBucket* m_pBucket;
        FBucket* m_pEnd;

        FIterator(FBucket* pBucket, FBucket* pEnd) : m_pBucket(pBucket), m_pEnd(pEnd)
        {
            SkipToOccupied();
        }

        void SkipToOccupied()
        {
            while (m_pBucket != m_pEnd && m_pBucket->State != EBucketState::Occupied)
            {
                m_pBucket++;
            }
        }

        FIterator& operator++()
        {
            m_pBucket++;
            SkipToOccupied();
            return *this;
        }

        bool operator!=(const FIterator& Other) const
        {
            return m_pBucket != Other.m_pBucket;
        }

        KeyType& operator*() const
        {
            return m_pBucket->Key;
        }
    };

    struct FConstIterator
    {
        const FBucket* m_pBucket;
        const FBucket* m_pEnd;

        FConstIterator(const FBucket* pBucket, const FBucket* pEnd) : m_pBucket(pBucket), m_pEnd(pEnd)
        {
            SkipToOccupied();
        }

        void SkipToOccupied()
        {
            while (m_pBucket != m_pEnd && m_pBucket->State != EBucketState::Occupied)
            {
                m_pBucket++;
            }
        }

        FConstIterator& operator++()
        {
            m_pBucket++;
            SkipToOccupied();
            return *this;
        }

        bool operator!=(const FConstIterator& Other) const
        {
            return m_pBucket != Other.m_pBucket;
        }

        const KeyType& operator*() const
        {
            return m_pBucket->Key;
        }
    };

    FIterator begin()
    {
        if (!m_pBuckets)
        {
            return FIterator(nullptr, nullptr);
        }

        return FIterator(m_pBuckets, m_pBuckets + m_Capacity);
    }

    FIterator end()
    {
        if (!m_pBuckets)
        {
            return FIterator(nullptr, nullptr);
        }

        return FIterator(m_pBuckets + m_Capacity, m_pBuckets + m_Capacity);
    }

    FConstIterator begin() const
    {
        if (!m_pBuckets)
        {
            return FConstIterator(nullptr, nullptr);
        }

        return FConstIterator(m_pBuckets, m_pBuckets + m_Capacity);
    }

    FConstIterator end() const
    {
        if (!m_pBuckets)
        {
            return FConstIterator(nullptr, nullptr);
        }

        return FConstIterator(m_pBuckets + m_Capacity, m_pBuckets + m_Capacity);
    }
};