#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/Containers/TMultiMap.h"

struct FNameEntry
{
    static const int32 NAME_SIZE = 64;

    wchar_t m_Name[NAME_SIZE];
};

class FNamePool
{
private:
    FNamePool();

    FNamePool(const FNamePool&) = delete;
    FNamePool& operator=(const FNamePool&) = delete;

    TArray<FNameEntry> m_Entries;
    TMultiMap<uint32, uint32> m_HashToIndex;

public:
    static FNamePool& Get();

    uint32 FindOrRegister(const wchar_t* Name);

    const wchar_t* GetEntryName(uint32 Index) const;

    int32 Num() const;

    static uint32 HashString(const wchar_t* Str);
};