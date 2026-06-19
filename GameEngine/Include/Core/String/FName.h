#pragma once

#include "EnginePCH.h"
#include "FString.h"
#include "Core/Containers/TArray.h"
#include "Core/Containers/HashFunctions.h"

class FName
{
private:
    static void EnsureInitialized()
    {
        if (!s_Entries.IsEmpty())
        {
            return;
        }

        s_Entries.Add(FString(L"None"));
    }

    static uint32 FindOrRegister(const FString& Name)
    {
        EnsureInitialized();

        for (int32 i = 0; i < s_Entries.Num(); i++)
        {
            if (s_Entries[i] == Name)
            {
                return (uint32)i;
            }
        }

        uint32 NewIndex = (uint32)s_Entries.Num();
        s_Entries.Add(Name);

        return NewIndex;
    }

    uint32 m_Index;

    static TArray<FString> s_Entries;

public:
    FName() : m_Index(0) 
    {

    }

    FName(const wchar_t* Name) : m_Index(0)
    {
        if (Name && Name[0] != L'\0')
        {
            m_Index = FindOrRegister(FString(Name));
        }
    }

    FName(const FString& Name) : m_Index(0)
    {
        if (!Name.IsEmpty())
        {
            m_Index = FindOrRegister(Name);
        }
    }

    bool operator==(const FName& Other) const 
    { 
        return m_Index == Other.m_Index; 
    }

    bool operator!=(const FName& Other) const 
    { 
        return m_Index != Other.m_Index; 
    }

    bool operator<(const FName& Other) const 
    { 
        return m_Index < Other.m_Index; 
    }

    const FString& ToString() const
    {
        EnsureInitialized();
        check(m_Index < (uint32)s_Entries.Num());

        return s_Entries[(int32)m_Index];
    }

    bool IsNone() const 
    { 
        return m_Index == 0; 
    }

    uint32 GetIndex() const 
    { 
        return m_Index; 
    }
};

inline uint32 GetTypeHash(const FName& Name)
{
    return GetTypeHash(Name.GetIndex());
}