#pragma once
#include "EnginePCH.h"
#include "TArray.h"
#include "TMap.h"

// TMultiMap: maps one key to multiple values using TMap<Key, TArray<Value>>
template<typename KeyType, typename ValueType>
class TMultiMap
{
private:
    TMap<KeyType, TArray<ValueType>> m_Map;

public:
    TMultiMap() 
    {

    }

    TMultiMap(const TMultiMap& Other) : m_Map(Other.m_Map) 
    {

    }

    TMultiMap(TMultiMap&& Other) noexcept : m_Map(MoveTemp(Other.m_Map)) 
    {

    }

    TMultiMap& operator=(const TMultiMap& Other)
    {
        m_Map = Other.m_Map;

        return *this;
    }

    TMultiMap& operator=(TMultiMap&& Other) noexcept
    {
        m_Map = MoveTemp(Other.m_Map);

        return *this;
    }

    // -------------------------------------------------------------------------
    // Insert
    // -------------------------------------------------------------------------

    void Add(const KeyType& Key, const ValueType& Value)
    {
        m_Map.FindOrAdd(Key).Add(Value);
    }

    // Add only if (Key, Value) pair does not already exist
    bool AddUnique(const KeyType& Key, const ValueType& Value)
    {
        TArray<ValueType>& Values = m_Map.FindOrAdd(Key);

        if (Values.Contains(Value))
        {
            return false;
        }

        Values.Add(Value);

        return true;
    }

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------

    const TArray<ValueType>* MultiFind(const KeyType& Key) const
    {
        return m_Map.Find(Key);
    }

    bool Contains(const KeyType& Key) const
    {
        const TArray<ValueType>* pValues = m_Map.Find(Key);

        return pValues != nullptr && !pValues->IsEmpty();
    }

    bool Contains(const KeyType& Key, const ValueType& Value) const
    {
        const TArray<ValueType>* pValues = m_Map.Find(Key);

        return pValues != nullptr && pValues->Contains(Value);
    }

    // -------------------------------------------------------------------------
    // Remove
    // -------------------------------------------------------------------------

    // Remove one occurrence of Value under Key
    bool RemoveSingle(const KeyType& Key, const ValueType& Value)
    {
        TArray<ValueType>* pValues = m_Map.Find(Key);

        if (!pValues)
        {
            return false;
        }

        bool Removed = pValues->Remove(Value);

        if (pValues->IsEmpty())
        {
            m_Map.Remove(Key);
        }

        return Removed;
    }

    // Remove all values under Key
    bool RemoveAll(const KeyType& Key)
    {
        return m_Map.Remove(Key);
    }

    void Reset()
    {
        m_Map.Reset();
    }

    void Empty()
    {
        m_Map.Empty();
    }

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    // Total number of keys (not values)
    int32 NumKeys() const
    {
        return m_Map.Num();
    }

    // Total number of values across all keys
    int32 Num() const
    {
        int32 Total = 0;

        for (const auto& Bucket : m_Map)
        {
            Total += Bucket.Value.Num();
        }

        return Total;
    }

    bool IsEmpty() const
    {
        return m_Map.IsEmpty();
    }
};