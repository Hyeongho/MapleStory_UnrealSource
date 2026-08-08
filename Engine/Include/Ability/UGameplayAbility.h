#pragma once
#include "EnginePCH.h"
#include "Object/UObject.h"
#include "Ability/FGameplayTagContainer.h"

class UAbilitySystemComponent;
class UGameplayEffect;

class UGameplayAbility : public UObject
{
    DECLARE_CLASS(UGameplayAbility, UObject)
public:
    UGameplayAbility();
    virtual ~UGameplayAbility() override = default;

    UGameplayEffect* m_pCostEffect;
    UGameplayEffect* m_pCooldownEffect;

    FGameplayTagContainer m_AbilityTags;
    FGameplayTagContainer m_ActivationRequiredTags;
    FGameplayTagContainer m_ActivationBlockedTags;

    virtual bool CanActivate(UAbilitySystemComponent* ASC) const;
    virtual void ActivateAbility(UAbilitySystemComponent* ASC);
    virtual void EndAbility(UAbilitySystemComponent* ASC);
};
