#pragma once
#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"
#include "HashFunctions.h"
#include "TSparseArray.h"

template<typename KeyType, typename ValueType>
class TMap
{
private:
    struct FMapElement
    {
        KeyType Key;
        ValueType Value;
        int32 m_HashNext;   // next element index in the same bucket chain

        FMapElement(const KeyType& InKey, const ValueType& InValue, int32 InHashNext)
            : Key(InKey), Value(InValue), m_HashNext(InHashNext)
        {
        }

        FMapElement(KeyType&& InKey, ValueType&& InValue, int32 InHashNext)
            : Key(MoveTemp(InKey)), Value(MoveTemp(InValue)), m_HashNext(InHashNext)
        {
        }

        FMapElement(const KeyType& InKey, int32 InHashNext)
            : Key(InKey), Value(), m_HashNext(InHashNext)
        {
        }
    };

    TSparseArray<FMapElement> m_Elements;
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

    // Relink every element into a fresh bucket array of NewNumBuckets slots.
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

    // Grow the bucket array when the load factor would exceed 1.0
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
    TMap() : m_pBuckets(nullptr), m_NumBuckets(0)
    {
    }

    TMap(const TMap& Other) : m_Elements(Other.m_Elements), m_pBuckets(nullptr), m_NumBuckets(0)
    {
        // Sparse indices are preserved by the structural copy, so the bucket
        // array can be copied verbatim.
        if (Other.m_pBuckets)
        {
            AllocateBuckets(Other.m_NumBuckets);
            FMemory::Memcpy(m_pBuckets, Other.m_pBuckets, sizeof(int32) * m_NumBuckets);
        }
    }

    TMap(TMap&& Other) noexcept : m_Elements(MoveTemp(Other.m_Elements)), m_pBuckets(Other.m_pBuckets), m_NumBuckets(Other.m_NumBuckets)
    {
        Other.m_pBuckets = nullptr;
        Other.m_NumBuckets = 0;
    }

    ~TMap()
    {
        Empty();
    }

    TMap& operator=(const TMap& Other)
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

    TMap& operator=(TMap&& Other) noexcept
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
    // Insert / Modify
    // -------------------------------------------------------------------------

    void Add(const KeyType& Key, const ValueType& Value)
    {
        ConditionalRehash(m_Elements.Num() + 1);

        int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

        for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
        {
            if (m_Elements[i].Key == Key)
            {
                m_Elements[i].Value = Value;
                return;
            }
        }

        Bucket = m_Elements.Emplace(Key, Value, Bucket);
    }

    void Add(KeyType&& Key, ValueType&& Value)
    {
        ConditionalRehash(m_Elements.Num() + 1);

        int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

        for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
        {
            if (m_Elements[i].Key == Key)
            {
                m_Elements[i].Value = MoveTemp(Value);
                return;
            }
        }

        Bucket = m_Elements.Emplace(MoveTemp(Key), MoveTemp(Value), Bucket);
    }

    ValueType& FindOrAdd(const KeyType& Key)
    {
        ConditionalRehash(m_Elements.Num() + 1);

        int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

        for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
        {
            if (m_Elements[i].Key == Key)
            {
                return m_Elements[i].Value;
            }
        }

        Bucket = m_Elements.Emplace(Key, Bucket);

        return m_Elements[Bucket].Value;
    }

    ValueType& operator[](const KeyType& Key)
    {
        return FindOrAdd(Key);
    }

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------

    ValueType* Find(const KeyType& Key) const
    {
        const int32 Index = FindIndex(Key);

        if (Index == INDEX_NONE)
        {
            return nullptr;
        }

        return const_cast<ValueType*>(&m_Elements[Index].Value);
    }

    ValueType& FindRef(const KeyType& Key) const
    {
        const int32 Index = FindIndex(Key);
        check(Index != INDEX_NONE);

        return const_cast<ValueType&>(m_Elements[Index].Value);
    }

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
    // Iterator - forwards to the sparse array, yields elements with .Key/.Value
    // -------------------------------------------------------------------------

    using FIterator = typename TSparseArray<FMapElement>::FIterator;
    using FConstIterator = typename TSparseArray<FMapElement>::FConstIterator;

    FIterator begin()
    {
        return m_Elements.begin();
    }

    FIterator end()
    {
        return m_Elements.end();
    }

    FConstIterator begin() const
    {
        return m_Elements.begin();
    }

    FConstIterator end() const
    {
        return m_Elements.end();
    }
};
