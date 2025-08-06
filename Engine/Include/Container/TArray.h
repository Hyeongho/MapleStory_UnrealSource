#pragma once

#include <cassert>
#include <initializer_list>
#include <cstring>
#include <algorithm>

template<typename T>
class TArray
{
public:
    TArray() : Data(nullptr), Size(0), Capacity(0)
    {
    }

    TArray(std::initializer_list<T> InitList) : TArray()
    {
        Reserve(InitList.size());
        for (const auto& Item : InitList)
        {
            Add(Item);
        }
    }

    ~TArray()
    {
        delete[] Data;
    }

    void Add(const T& Item)
    {
        ResizeIfNeeded();
        Data[Size++] = Item;
    }

    void InsertAt(size_t Index, const T& Item)
    {
        assert(Index <= Size);
        ResizeIfNeeded();

        for (size_t i = Size; i-- > Index; )
        {
            Data[i + 1] = Data[i];
        }

        Data[Index] = Item;
        Size++;
    }

    void RemoveAt(size_t Index)
    {
        assert(Index < Size);

        for (size_t i = Index; i + 1 < Size; i++)
        {
            Data[i] = Data[i + 1];
        }

        Size--;
    }

    T* Find(const T& Item)
    {
        for (size_t i = 0; i < Size; i++)
        {
            if (Data[i] == Item)
            {
                return &Data[i];
            }
        }

        return nullptr;
    }

    bool Contains(const T& Item) const
    {
        for (size_t i = 0; i < Size; i++)
        {
            if (Data[i] == Item)
            {
                return true;
            }
        }

        return false;
    }

    void Reserve(size_t NewCapacity)
    {
        if (NewCapacity <= Capacity)
        {
            return;
        }

        T* NewData = new T[NewCapacity];

        for (size_t i = 0; i < Size; i++)
        {
            NewData[i] = Data[i];
        }

        delete[] Data;
        Data = NewData;
        Capacity = NewCapacity;
    }

    void Empty()
    {
        Size = 0;
    }

    void Clear()
    {
        delete[] Data;
        Data = nullptr;
        Size = 0;
        Capacity = 0;
    }

    void Reverse()
    {
        for (size_t i = 0; i < Size / 2; i++)
        {
            std::swap(Data[i], Data[Size - i - 1]);
        }
    }

    void Sort(bool (*CompareFunc)(const T&, const T&))
    {
        std::sort(Data, Data + Size, CompareFunc);
    }

    size_t Num() const
    {
        return Size;
    }

    T& operator[](size_t Index)
    {
        assert(Index < Size);
        return Data[Index];
    }

    const T& operator[](size_t Index) const
    {
        assert(Index < Size);
        return Data[Index];
    }

    // Iterator support
    T* begin()
    {
        return Data;
    }

    T* end()
    {
        return Data + Size;
    }

    const T* begin() const
    {
        return Data;
    }

    const T* end() const
    {
        return Data + Size;
    }

private:
    T* Data;
    size_t Size;
    size_t Capacity;

    void ResizeIfNeeded()
    {
        if (Size >= Capacity)
        {
            size_t NewCapacity = (Capacity == 0) ? 4 : Capacity * 2;
            Reserve(NewCapacity);
        }
    }
};
