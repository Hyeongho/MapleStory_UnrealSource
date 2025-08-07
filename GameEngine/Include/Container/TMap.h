#pragma once

#include <unordered_map>
#include <initializer_list>
#include <cassert>

template<typename KeyType, typename ValueType>
class TMap
{
public:
    TMap() = default;

    TMap(std::initializer_list<std::pair<const KeyType, ValueType>> InitList)
    {
        for (const auto& Pair : InitList)
        {
            Map.insert(Pair);
        }
    }

    void Add(const KeyType& Key, const ValueType& Value)
    {
        Map[Key] = Value;
    }

    void Remove(const KeyType& Key)
    {
        Map.erase(Key);
    }

    ValueType* Find(const KeyType& Key)
    {
        auto It = Map.find(Key);
        if (It != Map.end())
        {
            return &It->second;
        }

        return nullptr;
    }

    const ValueType* Find(const KeyType& Key) const
    {
        auto It = Map.find(Key);
        if (It != Map.end())
        {
            return &It->second;
        }
        return nullptr;
    }

    bool Contains(const KeyType& Key) const
    {
        return Map.find(Key) != Map.end();
    }

    size_t Num() const
    {
        return Map.size();
    }

    ValueType& operator[](const KeyType& Key)
    {
        return Map[Key];
    }

    const ValueType& operator[](const KeyType& Key) const
    {
        auto It = Map.find(Key);
        assert(It != Map.end());
        return It->second;
    }

    auto begin()
    {
        return Map.begin();
    }

    auto end()
    {
        return Map.end();
    }

    auto begin() const
    {
        return Map.begin();
    }

    auto end() const
    {
        return Map.end();
    }

    void Clear()
    {
        Map.clear();
    }

private:
    std::unordered_map<KeyType, ValueType> Map;
};
