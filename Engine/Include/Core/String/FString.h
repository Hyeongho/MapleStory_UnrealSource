#pragma once

#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Containers/TArray.h"
#include <cwchar>
#include <cstdarg>

class FString
{
public:
    FString() : m_pData(nullptr), m_Length(0), m_Capacity(0) 
    {

    }

    FString(const wchar_t* Str) : m_pData(nullptr), m_Length(0), m_Capacity(0)
    {
        if (Str && Str[0] != L'\0')
        {
            m_Length = (int32)wcslen(Str);
            m_Capacity = m_Length + 1;
            m_pData = (wchar_t*)FMemory::Malloc(sizeof(wchar_t) * m_Capacity, alignof(wchar_t));

            check(m_pData != nullptr);

            FMemory::Memcpy(m_pData, Str, sizeof(wchar_t) * m_Capacity);
        }
    }

    FString(const FString& Other) : m_pData(nullptr), m_Length(0), m_Capacity(0)
    {
        *this = Other;
    }

    FString(FString&& Other) noexcept : m_pData(Other.m_pData), m_Length(Other.m_Length), m_Capacity(Other.m_Capacity)
    {
        Other.m_pData = nullptr;
        Other.m_Length = 0;
        Other.m_Capacity = 0;
    }

    ~FString()
    {
        if (m_pData)
        {
            FMemory::Free(m_pData);
            m_pData = nullptr;
        }
    }

private:
    wchar_t* m_pData;
    int32 m_Length;
    int32 m_Capacity;

    void Grow(int32 NewCapacity);

public:

    // -------------------------------------------------------------------------
    // Assignment
    // -------------------------------------------------------------------------

    FString& operator=(const FString& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        if (m_pData)
        {
            FMemory::Free(m_pData);
            m_pData = nullptr;
        }

        m_Length = 0;
        m_Capacity = 0;

        if (Other.m_Length > 0)
        {
            m_Length = Other.m_Length;
            m_Capacity = m_Length + 1;
            m_pData = (wchar_t*)FMemory::Malloc(sizeof(wchar_t) * m_Capacity, alignof(wchar_t));
            FMemory::Memcpy(m_pData, Other.m_pData, sizeof(wchar_t) * m_Capacity);
        }

        return *this;
    }

    FString& operator=(FString&& Other) noexcept
    {
        if (this == &Other)
        {
            return *this;
        }

        if (m_pData)
        {
            FMemory::Free(m_pData);
        }

        m_pData = Other.m_pData;
        m_Length = Other.m_Length;
        m_Capacity = Other.m_Capacity;

        Other.m_pData = nullptr;
        Other.m_Length = 0;
        Other.m_Capacity = 0;

        return *this;
    }

    FString& operator=(const wchar_t* Str)
    {
        *this = FString(Str);
        return *this;
    }

    // -------------------------------------------------------------------------
    // Append
    // -------------------------------------------------------------------------

    FString& operator+=(const FString& Other)
    {
        if (Other.m_Length == 0)
        {
            return *this;
        }

        int32 NewLength = m_Length + Other.m_Length;
        int32 NewCapacity = NewLength + 1;

        if (NewCapacity > m_Capacity)
        {
            int32 GrowTo = (m_Capacity * 2 > NewCapacity) ? m_Capacity * 2 : NewCapacity;
            Grow(GrowTo);
        }

        FMemory::Memcpy(m_pData + m_Length, Other.m_pData, sizeof(wchar_t) * (Other.m_Length + 1));
        m_Length = NewLength;
        return *this;
    }

    FString& operator+=(const wchar_t* Str)
    {
        if (!Str || Str[0] == L'\0')
        {
            return *this;
        }

        return *this += FString(Str);
    }

    FString& operator+=(wchar_t Ch)
    {
        int32 NewLength = m_Length + 1;
        int32 NewCapacity = NewLength + 1;

        if (NewCapacity > m_Capacity)
        {
            int32 GrowTo = (m_Capacity * 2 > NewCapacity) ? m_Capacity * 2 : NewCapacity;
            Grow(GrowTo);
        }

        else if (!m_pData)
        {
            Grow(NewCapacity);
        }

        m_pData[m_Length] = Ch;
        m_pData[m_Length + 1] = L'\0';
        m_Length = NewLength;

        return *this;
    }

    FString operator+(const FString& Other) const
    {
        FString Result(*this);
        Result += Other;
        return Result;
    }

    FString operator+(const wchar_t* Str) const
    {
        FString Result(*this);
        Result += Str;
        return Result;
    }

    // -------------------------------------------------------------------------
    // Comparison
    // -------------------------------------------------------------------------

    bool operator==(const FString& Other) const
    {
        if (m_Length != Other.m_Length)
        {
            return false;
        }

        if (m_Length == 0)
        {
            return true;
        }

        return wcscmp(m_pData, Other.m_pData) == 0;
    }

    bool operator!=(const FString& Other) const
    {
        return !(*this == Other);
    }

    bool operator<(const FString& Other) const
    {
        if (!m_pData && !Other.m_pData)
        {
            return false;
        }
        if (!m_pData)
        {
            return true;
        }
        if (!Other.m_pData)
        {
            return false;
        }
        return wcscmp(m_pData, Other.m_pData) < 0;
    }

    wchar_t& operator[](int32 Index)
    {
        check(m_pData && Index >= 0 && Index < m_Length);
        return m_pData[Index];
    }

    const wchar_t& operator[](int32 Index) const
    {
        check(m_pData && Index >= 0 && Index < m_Length);
        return m_pData[Index];
    }

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    int32 Len() const { return m_Length; }
    bool IsEmpty() const { return m_Length == 0; }

    const wchar_t* GetData() const
    {
        return m_pData ? m_pData : L"";
    }

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------

    bool Contains(const FString& Sub) const;
    bool StartsWith(const FString& Prefix) const;
    bool EndsWith(const FString& Suffix) const;

    // -------------------------------------------------------------------------
    // Transform
    // -------------------------------------------------------------------------

    FString ToUpper() const;
    FString ToLower() const;
    FString Substring(int32 Start, int32 Length) const;
    int32 Split(wchar_t Delim, TArray<FString>& OutParts) const;

    // -------------------------------------------------------------------------
    // Parse
    // -------------------------------------------------------------------------

    int32 ToInt() const;
    float ToFloat() const;

    // -------------------------------------------------------------------------
    // Static
    // -------------------------------------------------------------------------

    static FString Printf(const wchar_t* Fmt, ...);
};

