#include "FName.h"

std::unordered_map<tstring, size_t> FName::NameToIDMap;
std::unordered_map<size_t, tstring> FName::IDToNameMap;
size_t FName::NextID = 1;

FName::FName()
    : NameID(0)
{
}

FName::FName(const tchar* InName)
{
    NameID = RegisterName(tstring(InName));
}

FName::FName(const tstring& InName)
{
    NameID = RegisterName(InName);
}

bool FName::operator==(const FName& Other) const
{
    return NameID == Other.NameID;
}

bool FName::operator!=(const FName& Other) const
{
    return NameID != Other.NameID;
}

bool FName::operator<(const FName& Other) const
{
    return NameID < Other.NameID;
}

const tstring& FName::ToString() const
{
    return GetNameByID(NameID);
}

const tchar* FName::ToCStr() const
{
    return GetNameByID(NameID).c_str();
}

size_t FName::RegisterName(const tstring& InName)
{
    auto it = NameToIDMap.find(InName);
    if (it != NameToIDMap.end())
    {
        return it->second;
    }

    size_t NewID = NextID++;
    NameToIDMap[InName] = NewID;
    IDToNameMap[NewID] = InName;
    return NewID;
}

const tstring& FName::GetNameByID(size_t ID)
{
    static const tstring Empty =
#ifdef UNICODE
        L"";
#else
        "";
#endif

    auto it = IDToNameMap.find(ID);
    return (it != IDToNameMap.end()) ? it->second : Empty;
}
