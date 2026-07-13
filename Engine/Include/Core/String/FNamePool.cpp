#include "EnginePCH.h"
#include "FNamePool.h"
#include <cwchar>

FNamePool::FNamePool()
{
    // Entry 0 is reserved for "None" so a default FName maps to it.
    FindOrRegister(L"None");
}

FNamePool& FNamePool::Get()
{
    static FNamePool Instance;
    return Instance;
}

uint32 FNamePool::HashString(const wchar_t* Str)
{
    uint32 Hash = 5381;

    while (*Str)
    {
        Hash = ((Hash << 5) + Hash) ^ (uint32)(*Str);
        Str++;
    }

    return Hash;
}

uint32 FNamePool::FindOrRegister(const wchar_t* Name)
{
    check(Name != nullptr);

    const uint32 Hash = HashString(Name);

    const TArray<uint32>* pCandidates = m_HashToIndex.MultiFind(Hash);
    if (pCandidates)
    {
        for (int32 i = 0; i < pCandidates->Num(); i++)
        {
            const uint32 Index = (*pCandidates)[i];

            if (wcscmp(m_Entries[(int32)Index].m_Name, Name) == 0)
            {
                return Index;
            }
        }
    }

    const int32 Length = (int32)wcslen(Name);
    check(Length < FNameEntry::NAME_SIZE);

    FNameEntry Entry;
    FMemory::Memcpy(Entry.m_Name, Name, sizeof(wchar_t) * (Length + 1));

    const uint32 NewIndex = (uint32)m_Entries.Num();
    m_Entries.Add(Entry);
    m_HashToIndex.Add(Hash, NewIndex);

    return NewIndex;
}

const wchar_t* FNamePool::GetEntryName(uint32 Index) const
{
    check(Index < (uint32)m_Entries.Num());

    return m_Entries[(int32)Index].m_Name;
}

int32 FNamePool::Num() const
{
    return m_Entries.Num();
}
