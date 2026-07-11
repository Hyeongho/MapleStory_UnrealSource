#pragma once
#include "EnginePCH.h"
#include "Core/String/FName.h"

class FGameplayTag
{
public:
    FGameplayTag();
    explicit FGameplayTag(const wchar_t* TagName);
    explicit FGameplayTag(const FName& TagName);

    bool IsValid() const;

    bool operator==(const FGameplayTag& Other) const;
    bool operator!=(const FGameplayTag& Other) const;

    bool MatchesParent(const FGameplayTag& Parent) const;

    const FName& GetTagName() const;

private:
    FName m_TagName;
};