#include "EnginePCH.h"
#include "Ability/FGameplayTag.h"
#include "Core/String/FString.h"

FGameplayTag::FGameplayTag()
{
}

FGameplayTag::FGameplayTag(const wchar_t* TagName) : m_TagName(TagName)
{
}

FGameplayTag::FGameplayTag(const FName& TagName) : m_TagName(TagName)
{
}

bool FGameplayTag::IsValid() const
{
    return !m_TagName.IsNone();
}

bool FGameplayTag::operator==(const FGameplayTag& Other) const
{
    return m_TagName == Other.m_TagName;
}

bool FGameplayTag::operator!=(const FGameplayTag& Other) const
{
    return m_TagName != Other.m_TagName;
}

bool FGameplayTag::MatchesParent(const FGameplayTag& Parent) const
{
    if (!Parent.IsValid())
    {
        return false;
    }

    if (!IsValid())
    {
        return false;
    }

    FString ThisStr = m_TagName.ToString();
    FString ParentStr = Parent.m_TagName.ToString();

    if (ThisStr == ParentStr)
    {
        return true;
    }

    if (!ThisStr.StartsWith(ParentStr))
    {
        return false;
    }

    int32 ParentLen = ParentStr.Len();
    if (ThisStr.Len() > ParentLen && ThisStr[ParentLen] == L'.')
    {
        return true;
    }

    return false;
}

const FName& FGameplayTag::GetTagName() const
{
    return m_TagName;
}
