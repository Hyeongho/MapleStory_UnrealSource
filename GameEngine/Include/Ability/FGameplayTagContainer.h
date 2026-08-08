#pragma once
#include "EnginePCH.h"
#include "Ability/FGameplayTag.h"
#include "Core/Containers/TArray.h"

class FGameplayTagContainer
{
public:
    void AddTag(const FGameplayTag& Tag);
    bool RemoveTag(const FGameplayTag& Tag);
    void Reset();

    bool HasTag(const FGameplayTag& Tag) const;
    bool HasParentTag(const FGameplayTag& Parent) const;
    bool HasAnyTag(const FGameplayTagContainer& Other) const;
    bool HasAllTags(const FGameplayTagContainer& Other) const;

    int32 Num() const;
    bool IsEmpty() const;

    const TArray<FGameplayTag>& GetTags() const;

private:
    TArray<FGameplayTag> m_Tags;
};
