#include "EnginePCH.h"
#include "Ability/UGameplayAbility.h"
#include "Ability/UAbilitySystemComponent.h"

UGameplayAbility::UGameplayAbility()
    : m_pCostEffect(nullptr)
    , m_pCooldownEffect(nullptr)
{
}

bool UGameplayAbility::CanActivate(UAbilitySystemComponent* ASC) const
{
    if (!ASC)
    {
        return false;
    }

    if (!m_ActivationBlockedTags.IsEmpty())
    {
        const TArray<FGameplayTag>& BlockedTags = m_ActivationBlockedTags.GetTags();
        for (int32 i = 0; i < BlockedTags.Num(); ++i)
        {
            if (ASC->HasTag(BlockedTags[i]))
            {
                return false;
            }
        }
    }

    if (!m_ActivationRequiredTags.IsEmpty())
    {
        const TArray<FGameplayTag>& RequiredTags = m_ActivationRequiredTags.GetTags();
        for (int32 i = 0; i < RequiredTags.Num(); ++i)
        {
            if (!ASC->HasTag(RequiredTags[i]))
            {
                return false;
            }
        }
    }

    return true;
}

void UGameplayAbility::ActivateAbility(UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        return;
    }

    if (m_pCostEffect)
    {
        ASC->ApplyGameplayEffect(m_pCostEffect);
    }

    if (m_pCooldownEffect)
    {
        ASC->ApplyGameplayEffect(m_pCooldownEffect);
    }
}

void UGameplayAbility::EndAbility(UAbilitySystemComponent* ASC)
{
    (void)ASC;
}
