#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/Containers/TMultiMap.h"

// Fixed-size name entry. The string is stored inline (no separate heap block),
// so entry lookup never chases an FString data pointer.
struct FNameEntry
{
    static const int32 NAME_SIZE = 64;

    wchar_t m_Name[NAME_SIZE];
};

// Global name pool. Maps string content to a stable uint32 index.
// Registration and lookup are O(1) via a content-hash (djb2) index,
// with wcscmp verification to survive hash collisions.
class FNamePool
{
public:
    static FNamePool& Get();

    // Returns the index of Name, registering it if it is not in the pool yet.
    uint32 FindOrRegister(const wchar_t* Name);

    const wchar_t* GetEntryName(uint32 Index) const;

    int32 Num() const;

    // djb2 hash over wide characters
    static uint32 HashString(const wchar_t* Str);

private:
    FNamePool();

    FNamePool(const FNamePool&) = delete;
    FNamePool& operator=(const FNamePool&) = delete;

    TArray<FNameEntry> m_Entries;
    TMultiMap<uint32, uint32> m_HashToIndex;
};
