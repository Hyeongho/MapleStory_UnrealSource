#pragma once
#include "EnginePCH.h"
#include "Object/UActorComponent.h"
#include "Ability/AbilityTypes.h"
#include "Ability/FGameplayTag.h"
#include "Ability/FGameplayTagContainer.h"
#include "Core/Containers/TArray.h"

class UAttributeSet;
class UGameplayEffect;
class UGameplayAbility;

class UAbilitySystemComponent : public UActorComponent
{
    DECLARE_CLASS(UAbilitySystemComponent, UActorComponent)
public:
    UAbilitySystemComponent();
    virtual ~UAbilitySystemComponent() override;

    void           SetAttributeSet(UAttributeSet* pSet);
    UAttributeSet* GetAttributeSet() const;
    float GetAttributeCurrentValue(const FName& Name) const;
    float GetAttributeBaseValue(const FName& Name)    const;

    bool HasTag(const FGameplayTag& Tag)       const;
    bool HasParentTag(const FGameplayTag& Tag) const;
    void AddLooseTag(const FGameplayTag& Tag);
    void RemoveLooseTag(const FGameplayTag& Tag);

    bool ApplyGameplayEffect(UGameplayEffect* pEffect);
    void RemoveEffectsWithTag(const FGameplayTag& Tag);
    void RemoveEffectsOfClass(UGameplayEffect* pEffect);

    int32 GrantAbility(UGameplayAbility* pAbility, int32 Level = 1);
    bool  TryActivateAbility(int32 SpecIndex);
    bool  TryActivateAbilityByTag(const FGameplayTag& AbilityTag);

    virtual void Tick(float DeltaTime) override;

private:
    UAttributeSet*                m_pAttributeSet;
    TArray<FActiveGameplayEffect> m_ActiveEffects;
    TArray<FGameplayAbilitySpec>  m_GrantedAbilities;
    FGameplayTagContainer         m_ActiveTags;

    void ApplyModifiersToBase(const TArray<FGameplayEffectModifier>& Mods, int32 Stacks = 1);
    void RecalculateAttributes();
    void TickActiveEffects(float DeltaTime);
    bool TryStackEffect(UGameplayEffect* pEffect);
};
