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

    // m_Tags와 인덱스가 항상 정렬되는 참조 카운트 — 몇 개의 소스(GameplayEffect
    // 등)가 이 태그를 부여했는지. AddTag/RemoveTag가 카운트를 증감시키고, 0이
    // 됐을 때만 실제로 m_Tags에서 제거한다 — 여러 이펙트가 같은 태그를 부여한
    // 상태에서 그중 하나만 만료돼도 다른 이펙트가 부여 중인 태그가 사라지지
    // 않도록 하기 위함. HasTag/GetTags() 등 조회 API는 m_Tags만 보므로(카운트는
    // 내부 구현일 뿐) 기존 동작·시그니처와 동일하게 유지된다.
    TArray<int32> m_TagCounts;
};