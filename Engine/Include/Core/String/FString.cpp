#include "EnginePCH.h"
#include "FString.h"
#include <cwchar>
#include <cstdarg>

void FString::Grow(int32 NewCapacity)
{
    wchar_t* pNew = (wchar_t*)FMemory::Malloc(sizeof(wchar_t) * NewCapacity, alignof(wchar_t));

    if (m_pData)
    {
        FMemory::Memcpy(pNew, m_pData, sizeof(wchar_t) * (m_Length + 1));
        FMemory::Free(m_pData);
    }

    else
    {
        pNew[0] = L'\0';
    }

    m_pData = pNew;
    m_Capacity = NewCapacity;
}

bool FString::Contains(const FString& Sub) const
{
    if (Sub.m_Length == 0 || m_Length < Sub.m_Length)
    {
        return Sub.m_Length == 0;
    }

    return wcsstr(m_pData, Sub.m_pData) != nullptr;
}

bool FString::StartsWith(const FString& Prefix) const
{
    if (Prefix.m_Length == 0)
    {
        return true;
    }

    if (m_Length < Prefix.m_Length)
    {
        return false;
    }

    return wcsncmp(m_pData, Prefix.m_pData, (size_t)Prefix.m_Length) == 0;
}

bool FString::EndsWith(const FString& Suffix) const
{
    if (Suffix.m_Length == 0)
    {
        return true;
    }

    if (m_Length < Suffix.m_Length)
    {
        return false;
    }

    return wcscmp(m_pData + (m_Length - Suffix.m_Length), Suffix.m_pData) == 0;
}

FString FString::ToUpper() const
{
    FString Result(*this);

    if (Result.m_pData)
    {
        for (int32 i = 0; i < Result.m_Length; i++)
        {
            if (Result.m_pData[i] >= L'a' && Result.m_pData[i] <= L'z')
            {
                Result.m_pData[i] = Result.m_pData[i] - L'a' + L'A';
            }
        }
    }

    return Result;
}

FString FString::ToLower() const
{
    FString Result(*this);

    if (Result.m_pData)
    {
        for (int32 i = 0; i < Result.m_Length; i++)
        {
            if (Result.m_pData[i] >= L'A' && Result.m_pData[i] <= L'Z')
            {
                Result.m_pData[i] = Result.m_pData[i] - L'A' + L'a';
            }
        }
    }

    return Result;
}

FString FString::Substring(int32 Start, int32 Length) const
{
    if (!m_pData || Start < 0 || Start >= m_Length || Length <= 0)
    {
        return FString();
    }

    int32 ActualLen = (Start + Length > m_Length) ? (m_Length - Start) : Length;
    FString Result;
    Result.m_Length = ActualLen;
    Result.m_Capacity = ActualLen + 1;
    Result.m_pData = (wchar_t*)FMemory::Malloc(sizeof(wchar_t) * Result.m_Capacity, alignof(wchar_t));
    FMemory::Memcpy(Result.m_pData, m_pData + Start, sizeof(wchar_t) * ActualLen);
    Result.m_pData[ActualLen] = L'\0';

    return Result;
}

int32 FString::Split(wchar_t Delim, TArray<FString>& OutParts) const
{
    OutParts.Reset();

    if (!m_pData || m_Length == 0)
    {
        return 0;
    }

    int32 Start = 0;

    for (int32 i = 0; i <= m_Length; i++)
    {
        if (i == m_Length || m_pData[i] == Delim)
        {
            OutParts.Add(Substring(Start, i - Start));
            Start = i + 1;
        }
    }

    return OutParts.Num();
}

int32 FString::ToInt() const
{
    if (!m_pData)
    {
        return 0;
    }

    return (int32)wcstol(m_pData, nullptr, 10);
}

float FString::ToFloat() const
{
    if (!m_pData)
    {
        return 0.f;
    }

    return wcstof(m_pData, nullptr);
}

FString FString::Printf(const wchar_t* Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);

    wchar_t Buffer[4096];
    int32 Len = (int32)vswprintf(Buffer, 4096, Fmt, Args);

    va_end(Args);

    if (Len <= 0)
    {
        return FString();
    }

    return FString(Buffer);
}