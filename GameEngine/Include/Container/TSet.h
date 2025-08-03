#pragma once

#include <unordered_set>
#include <initializer_list>
#include <cassert>

template<typename T>
class TSet
{
public:
    TSet() = default;

    TSet(std::initializer_list<T> InitList)
    {
        for (const auto& Item : InitList)
        {
            Set.insert(Item);
        }
    }

    void Add(const T& Element)
    {
        Set.insert(Element);
    }

    void Remove(const T& Element)
    {
        Set.erase(Element);
    }

    bool Contains(const T& Element) const
    {
        return Set.find(Element) != Set.end();
    }

    T* Find(const T& Element)
    {
        auto It = Set.find(Element);

        if (It != Set.end())
        {
            return const_cast<T*>(&(*It));
        }

        return nullptr;
    }

    const T* Find(const T& Element) const
    {
        auto It = Set.find(Element);

        if (It != Set.end())
        {
            return &(*It);
        }

        return nullptr;
    }

    size_t Num() const
    {
        return Set.size();
    }

    auto begin()
    {
        return Set.begin();
    }

    auto end()
    {
        return Set.end();
    }

    auto begin() const
    {
        return Set.begin();
    }

    auto end() const
    {
        return Set.end();
    }

private:
    std::unordered_set<T> Set;
};
