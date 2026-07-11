#pragma once
#include "EnginePCH.h"
#include "Object/UObject.h"
#include "Ability/AbilityTypes.h"
#include "Ability/FGameplayTagContainer.h"
#include "Core/Containers/TArray.h"

class UGameplayEffect : public UObject
{
    DECLARE_CLASS(UGameplayEffect, UObject)
public:
    UGameplayEffect();
    virtual ~UGameplayEffect() override = default;

    EGameplayEffectDurationType m_DurationType = EGameplayEffectDurationType::Instant;
    float m_Duration = 0.f;
    float m_Period = 0.f;
    int32 m_MaxStacks = 1;

    TArray<FGameplayEffectModifier> m_Modifiers;
    FGameplayTagContainer m_GrantedTags;
    FGameplayTagContainer m_RequiredTargetTags;
    FGameplayTagContainer m_BlockedTargetTags;

    void AddModifier(const FName& Attr, EGameplayModifierOperation Op, float Mag);
    void AddGrantedTag(const FGameplayTag& Tag);
};