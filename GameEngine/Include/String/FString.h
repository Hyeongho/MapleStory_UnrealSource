#pragma once

#include <string>
#include <cstring>

#ifdef UNICODE
using tchar = wchar_t;
using tstring = std::wstring;
#else
using tchar = char;
using tstring = std::string;
#endif

class FString
{
public:
    FString() {}

    FString(const tchar* InStr)
        : Str(InStr)
    {
    }

    FString(const tstring& InStr)
        : Str(InStr)
    {
    }

#ifdef UNICODE
    FString(const std::string& InAnsi)
    {
        Str = std::wstring(InAnsi.begin(), InAnsi.end());
    }
#else
    FString(const std::wstring& InWide)
    {
        Str = std::string(InWide.begin(), InWide.end());
    }
#endif

    const tchar* c_str() const
    {
        return Str.c_str();
    }

    const tstring& ToString() const
    {
        return Str;
    }

    size_t Length() const
    {
        return Str.length();
    }

    bool IsEmpty() const
    {
        return Str.empty();
    }

    void Clear()
    {
        Str.clear();
    }

    void Append(const FString& Other)
    {
        Str += Other.Str;
    }

    void Append(const tstring& Other)
    {
        Str += Other;
    }

    void Append(const tchar* Other)
    {
        Str += Other;
    }

    bool operator==(const FString& Other) const
    {
        return Str == Other.Str;
    }

    bool operator!=(const FString& Other) const
    {
        return Str != Other.Str;
    }

    FString operator+(const FString& Other) const
    {
        return FString(Str + Other.Str);
    }

    FString& operator+=(const FString& Other)
    {
        Str += Other.Str;
        return *this;
    }

private:
    tstring Str;
};
