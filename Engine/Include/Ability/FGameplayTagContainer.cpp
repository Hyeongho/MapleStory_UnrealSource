#include "EnginePCH.h"
#include "Ability/FGameplayTagContainer.h"

void FGameplayTagContainer::AddTag(const FGameplayTag& Tag)
{
    if (!Tag.IsValid())
    {
        return;
    }

    if (!HasTag(Tag))
    {
        m_Tags.Add(Tag);
    }
}

bool FGameplayTagContainer::RemoveTag(const FGameplayTag& Tag)
{
    for (int32 i = 0; i < m_Tags.Num(); i++)
    {
        if (m_Tags[i] == Tag)
        {
            m_Tags.RemoveAtSwap(i);
            return true;
        }
    }

    return false;
}

void FGameplayTagContainer::Reset()
{
    m_Tags.Reset();
}

bool FGameplayTagContainer::HasTag(const FGameplayTag& Tag) const
{
    for (int32 i = 0; i < m_Tags.Num(); i++)
    {
        if (m_Tags[i] == Tag)
        {
            return true;
        }
    }

    return false;
}

bool FGameplayTagContainer::HasParentTag(const FGameplayTag& Parent) const
{
    for (int32 i = 0; i < m_Tags.Num(); i++)
    {
        if (m_Tags[i].MatchesParent(Parent))
        {
            return true;
        }
    }

    return false;
}

bool FGameplayTagContainer::HasAnyTag(const FGameplayTagContainer& Other) const
{
    for (int32 i = 0; i < Other.m_Tags.Num(); i++)
    {
        if (HasTag(Other.m_Tags[i]))
        {
            return true;
        }
    }

    return false;
}

bool FGameplayTagContainer::HasAllTags(const FGameplayTagContainer& Other) const
{
    for (int32 i = 0; i < Other.m_Tags.Num(); i++)
    {
        if (!HasTag(Other.m_Tags[i]))
        {
            return false;
        }
    }

    return true;
}

int32 FGameplayTagContainer::Num() const
{
    return m_Tags.Num();
}

bool FGameplayTagContainer::IsEmpty() const
{
    return m_Tags.Num() == 0;
}

const TArray<FGameplayTag>& FGameplayTagContainer::GetTags() const
{
    return m_Tags;
}
