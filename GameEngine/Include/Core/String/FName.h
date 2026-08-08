#pragma once

#include "EnginePCH.h"
#include "FString.h"
#include "FNamePool.h"
#include "Core/Containers/HashFunctions.h"

class FName
{
public:
    FName() : m_Index(0) 
    {

    }

    FName(const wchar_t* Name) : m_Index(0)
    {
        if (Name && Name[0] != L'\0')
        {
            m_Index = FNamePool::Get().FindOrRegister(Name);
        }
    }

    FName(const FString& Name) : m_Index(0)
    {
        if (!Name.IsEmpty())
        {
            m_Index = FNamePool::Get().FindOrRegister(Name.GetData());
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

    FString ToString() const
    {
        const wchar_t* pEntryName = FNamePool::Get().GetEntryName(m_Index);
        return FString(pEntryName);
    }

    bool IsNone() const 
    { 
        return m_Index == 0; 
    }

    uint32 GetIndex() const 
    { 
        return m_Index; 
    }

private:
    uint32 m_Index;
};

inline uint32 GetTypeHash(const FName& Name)
{
    return GetTypeHash(Name.GetIndex());
}
