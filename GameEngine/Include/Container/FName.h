#pragma once

#include <string>
#include <unordered_map>
#include <cstring>

#ifdef UNICODE
using tchar = wchar_t;
using tstring = std::wstring;
#else
using tchar = char;
using tstring = std::string;
#endif

class FName
{
public:
    FName();
    explicit FName(const tstring& InName);
    explicit FName(const tchar* InName);

#ifdef UNICODE
    explicit FName(const char* InAnsi) : FName(tstring(InAnsi, InAnsi + std::strlen(InAnsi))) 
    {
    }

    explicit FName(const std::string& InAnsi) : FName(tstring(InAnsi.begin(), InAnsi.end())) 
    {
    }
#else
    explicit FName(const wchar_t* InWide) : FName(tstring(InWide, InWide + std::wcslen(InWide))) 
    {
    }

    explicit FName(const std::wstring& InWide) : FName(tstring(InWide.begin(), InWide.end())) 
    {
    }
#endif

    bool operator==(const FName& Other) const;
    bool operator!=(const FName& Other) const;
    bool operator<(const FName& Other) const;

    const tstring& ToString() const;
    const tchar* ToCStr() const;

    size_t GetID() const 
    { 
        return NameID; 
    }

private:
    size_t NameID;

    static std::unordered_map<tstring, size_t> NameToIDMap;
    static std::unordered_map<size_t, tstring> IDToNameMap;
    static size_t NextID;

    static size_t RegisterName(const tstring& InName);
    static const tstring& GetNameByID(size_t ID);
};
