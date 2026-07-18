#pragma once
#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"
#include "HashFunctions.h"
#include "TSparseArray.h"

template<typename KeyType>
class TSet
{
private:
    struct FSetElement
    {
        KeyType Key;
        int32 m_HashNext;   // next element index in the same bucket chain

        FSetElement(const KeyType& InKey, int32 InHashNext) : Key(InKey), m_HashNext(InHashNext)
        {
        }

        FSetElement(KeyType&& InKey, int32 InHashNext) : Key(MoveTemp(InKey)), m_HashNext(InHashNext)
        {
        }
    };

    TSparseArray<FSetElement> m_Elements;
    int32* m_pBuckets;      // element index heads, INDEX_NONE-terminated chains
    int32 m_NumBuckets;     // always a power of two

    static const int32 INITIAL_BUCKETS = 16;

    int32 BucketIndex(uint32 Hash) const
    {
        return (int32)(Hash & (uint32)(m_NumBuckets - 1));
    }

    void AllocateBuckets(int32 NumBuckets)
    {
        m_pBuckets = (int32*)FMemory::Malloc(sizeof(int32) * NumBuckets, alignof(int32));
        m_NumBuckets = NumBuckets;

        for (int32 i = 0; i < NumBuckets; i++)
        {
            m_pBuckets[i] = INDEX_NONE;
        }
    }

    void Rehash(int32 NewNumBuckets)
    {
        if (m_pBuckets)
        {
            FMemory::Free(m_pBuckets);
        }

        AllocateBuckets(NewNumBuckets);

        for (auto It = m_Elements.begin(); It != m_Elements.end(); ++It)
        {
            int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(It->Key))];
            It->m_HashNext = Bucket;
            Bucket = It.GetIndex();
        }
    }

    void ConditionalRehash(int32 NumElements)
    {
        if (!m_pBuckets)
        {
            AllocateBuckets(INITIAL_BUCKETS);
            return;
        }

        if (NumElements > m_NumBuckets)
        {
            int32 NewNumBuckets = m_NumBuckets;

            while (NumElements > NewNumBuckets)
            {
                NewNumBuckets *= 2;
            }

            Rehash(NewNumBuckets);
        }
    }

    int32 FindIndex(const KeyType& Key) const
    {
        if (!m_pBuckets || m_Elements.IsEmpty())
        {
            return INDEX_NONE;
        }

        for (int32 i = m_pBuckets[BucketIndex(GetTypeHash(Key))]; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
        {
            if (m_Elements[i].Key == Key)
            {
                return i;
            }
        }

        return INDEX_NONE;
    }

public:
    TSet() : m_pBuckets(nullptr), m_NumBuckets(0)
    {
    }

    TSet(const TSet& Other) : m_Elements(Other.m_Elements), m_pBuckets(nullptr), m_NumBuckets(0)
    {
        if (Other.m_pBuckets)
        {
            AllocateBuckets(Other.m_NumBuckets);
            FMemory::Memcpy(m_pBuckets, Other.m_pBuckets, sizeof(int32) * m_NumBuckets);
        }
    }

    TSet(TSet&& Other) noexcept : m_Elements(MoveTemp(Other.m_Elements)), m_pBuckets(Other.m_pBuckets), m_NumBuckets(Other.m_NumBuckets)
    {
        Other.m_pBuckets = nullptr;
        Other.m_NumBuckets = 0;
    }

    ~TSet()
    {
        Empty();
    }

    TSet& operator=(const TSet& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        Empty();

        m_Elements = Other.m_Elements;

        if (Other.m_pBuckets)
        {
            AllocateBuckets(Other.m_NumBuckets);
            FMemory::Memcpy(m_pBuckets, Other.m_pBuckets, sizeof(int32) * m_NumBuckets);
        }

        return *this;
    }

    TSet& operator=(TSet&& Other) noexcept
    {
        if (this == &Other)
        {
            return *this;
        }

        Empty();

        m_Elements = MoveTemp(Other.m_Elements);
        m_pBuckets = Other.m_pBuckets;
        m_NumBuckets = Other.m_NumBuckets;

        Other.m_pBuckets = nullptr;
        Other.m_NumBuckets = 0;

        return *this;
    }

    // -------------------------------------------------------------------------
    // Insert
    // -------------------------------------------------------------------------

    bool Add(const KeyType& Key)
    {
        ConditionalRehash(m_Elements.Num() + 1);

        int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

        for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
        {
            if (m_Elements[i].Key == Key)
            {
                return false; // already exists
            }
        }

        Bucket = m_Elements.Emplace(Key, Bucket);

        return true;
    }

    bool Add(KeyType&& Key)
    {
        ConditionalRehash(m_Elements.Num() + 1);

        int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

        for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
        {
            if (m_Elements[i].Key == Key)
            {
                return false; // already exists
            }
        }

        Bucket = m_Elements.Emplace(MoveTemp(Key), Bucket);

        return true;
    }

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------

    bool Contains(const KeyType& Key) const
    {
        return FindIndex(Key) != INDEX_NONE;
    }

    // -------------------------------------------------------------------------
    // Remove
    // -------------------------------------------------------------------------

    bool Remove(const KeyType& Key)
    {
        if (!m_pBuckets || m_Elements.IsEmpty())
        {
            return false;
        }

        int32* pLink = &m_pBuckets[BucketIndex(GetTypeHash(Key))];

        while (*pLink != INDEX_NONE)
        {
            const int32 Index = *pLink;

            if (m_Elements[Index].Key == Key)
            {
                *pLink = m_Elements[Index].m_HashNext;
                m_Elements.RemoveAt(Index);
                return true;
            }

            pLink = &m_Elements[Index].m_HashNext;
        }

        return false;
    }

    void Reset()
    {
        m_Elements.Reset();

        for (int32 i = 0; i < m_NumBuckets; i++)
        {
            m_pBuckets[i] = INDEX_NONE;
        }
    }

    void Empty()
    {
        m_Elements.Empty();

        if (m_pBuckets)
        {
            FMemory::Free(m_pBuckets);
            m_pBuckets = nullptr;
        }

        m_NumBuckets = 0;
    }

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    int32 Num() const
    {
        return m_Elements.Num();
    }

    bool IsEmpty() const
    {
        return m_Elements.IsEmpty();
    }

    // -------------------------------------------------------------------------
    // Iterator - forwards to the sparse array, yields KeyType&
    // -------------------------------------------------------------------------

    struct FIterator
    {
        typename TSparseArray<FSetElement>::FIterator m_It;

        FIterator(typename TSparseArray<FSetElement>::FIterator It) : m_It(It)
        {
        }

        FIterator& operator++()
        {
            ++m_It;
            return *this;
        }

        bool operator!=(const FIterator& Other) const
        {
            return m_It != Other.m_It;
        }

        KeyType& operator*() const
        {
            return (*m_It).Key;
        }
    };

    struct FConstIterator
    {
        typename TSparseArray<FSetElement>::FConstIterator m_It;

        FConstIterator(typename TSparseArray<FSetElement>::FConstIterator It) : m_It(It)
        {
        }

        FConstIterator& operator++()
        {
            ++m_It;
            return *this;
        }

        bool operator!=(const FConstIterator& Other) const
        {
            return m_It != Other.m_It;
        }

        const KeyType& operator*() const
        {
            return (*m_It).Key;
        }
    };

    FIterator begin()
    {
        return FIterator(m_Elements.begin());
    }

    FIterator end()
    {
        return FIterator(m_Elements.end());
    }

    FConstIterator begin() const
    {
        return FConstIterator(m_Elements.begin());
    }

    FConstIterator end() const
    {
        return FConstIterator(m_Elements.end());
    }
};