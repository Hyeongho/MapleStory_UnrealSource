#include "EnginePCH.h"
#include "Ability/UAbilitySystemComponent.h"
#include "Ability/UAttributeSet.h"
#include "Ability/UGameplayEffect.h"
#include "Ability/UGameplayAbility.h"
#include "Core/Math/FMath.h"

UAbilitySystemComponent::UAbilitySystemComponent() : m_pAttributeSet(nullptr)
{
}

UAbilitySystemComponent::~UAbilitySystemComponent() = default;

// ---------------------------------------------------------------------------
// �Ӽ�
// ---------------------------------------------------------------------------

void UAbilitySystemComponent::SetAttributeSet(UAttributeSet* pSet)
{
    m_pAttributeSet = pSet;
}

UAttributeSet* UAbilitySystemComponent::GetAttributeSet() const
{
    return m_pAttributeSet;
}

float UAbilitySystemComponent::GetAttributeCurrentValue(const FName& Name) const
{
    if (!m_pAttributeSet)
    {
        return 0.f;
    }
    return m_pAttributeSet->GetCurrentValue(Name);
}

float UAbilitySystemComponent::GetAttributeBaseValue(const FName& Name) const
{
    if (!m_pAttributeSet)
    {
        return 0.f;
    }
    return m_pAttributeSet->GetBaseValue(Name);
}

// ---------------------------------------------------------------------------
// �±�
// ---------------------------------------------------------------------------

bool UAbilitySystemComponent::HasTag(const FGameplayTag& Tag) const
{
    return m_ActiveTags.HasTag(Tag);
}

bool UAbilitySystemComponent::HasParentTag(const FGameplayTag& Tag) const
{
    return m_ActiveTags.HasParentTag(Tag);
}

void UAbilitySystemComponent::AddLooseTag(const FGameplayTag& Tag)
{
    m_ActiveTags.AddTag(Tag);
}

void UAbilitySystemComponent::RemoveLooseTag(const FGameplayTag& Tag)
{
    m_ActiveTags.RemoveTag(Tag);
}

// ---------------------------------------------------------------------------
// ȿ�� ����
// ---------------------------------------------------------------------------

bool UAbilitySystemComponent::ApplyGameplayEffect(UGameplayEffect* pEffect)
{
    if (!pEffect)
    {
        return false;
    }

    if (pEffect->m_DurationType == EGameplayEffectDurationType::Instant)
    {
        ApplyModifiersToBase(pEffect->m_Modifiers, 1);
        return true;
    }

    if (TryStackEffect(pEffect))
    {
        RecalculateAttributes();
        return true;
    }

    FActiveGameplayEffect Active;
    Active.m_pSpec = pEffect;
    Active.m_Duration = pEffect->m_Duration;
    Active.m_PeriodTimer = pEffect->m_Period;
    Active.m_StackCount = 1;

    m_ActiveEffects.Add(Active);

    const TArray<FGameplayTag>& GrantedTags = pEffect->m_GrantedTags.GetTags();
    for (int32 i = 0; i < GrantedTags.Num(); i++)
    {
        m_ActiveTags.AddTag(GrantedTags[i]);
    }

    RecalculateAttributes();
    return true;
}

bool UAbilitySystemComponent::TryStackEffect(UGameplayEffect* pEffect)
{
    if (pEffect->m_MaxStacks <= 1)
    {
        return false;
    }

    for (int32 i = 0; i < m_ActiveEffects.Num(); i++)
    {
        if (m_ActiveEffects[i].m_pSpec == pEffect)
        {
            if (m_ActiveEffects[i].m_StackCount < pEffect->m_MaxStacks)
            {
                m_ActiveEffects[i].m_StackCount++;
                m_ActiveEffects[i].m_Duration = pEffect->m_Duration;
            }
            return true;
        }
    }
    return false;
}

void UAbilitySystemComponent::RemoveEffectsWithTag(const FGameplayTag& Tag)
{
    for (int32 i = m_ActiveEffects.Num() - 1; i >= 0; i--)
    {
        UGameplayEffect* pSpec = m_ActiveEffects[i].m_pSpec;
        if (pSpec && pSpec->m_GrantedTags.HasTag(Tag))
        {
            const TArray<FGameplayTag>& GrantedTags = pSpec->m_GrantedTags.GetTags();
            for (int32 t = 0; t < GrantedTags.Num(); t++)
            {
                m_ActiveTags.RemoveTag(GrantedTags[t]);
            }

            m_ActiveEffects.RemoveAtSwap(i);
        }
    }
    RecalculateAttributes();
}

void UAbilitySystemComponent::RemoveEffectsOfClass(UGameplayEffect* pEffect)
{
    for (int32 i = m_ActiveEffects.Num() - 1; i >= 0; i--)
    {
        if (m_ActiveEffects[i].m_pSpec == pEffect)
        {
            const TArray<FGameplayTag>& GrantedTags = pEffect->m_GrantedTags.GetTags();

            for (int32 t = 0; t < GrantedTags.Num(); t++)
            {
                m_ActiveTags.RemoveTag(GrantedTags[t]);
            }

            m_ActiveEffects.RemoveAtSwap(i);
        }
    }

    RecalculateAttributes();
}

// ---------------------------------------------------------------------------
// ��ų
// ---------------------------------------------------------------------------

int32 UAbilitySystemComponent::GrantAbility(UGameplayAbility* pAbility, int32 Level)
{
    FGameplayAbilitySpec Spec;
    Spec.m_pAbility = pAbility;
    Spec.m_Level = Level;
    Spec.m_bIsActive = false;
    m_GrantedAbilities.Add(Spec);
    return m_GrantedAbilities.Num() - 1;
}

bool UAbilitySystemComponent::TryActivateAbility(int32 SpecIndex)
{
    if (SpecIndex < 0 || SpecIndex >= m_GrantedAbilities.Num())
    {
        return false;
    }

    FGameplayAbilitySpec& Spec = m_GrantedAbilities[SpecIndex];
    if (!Spec.m_pAbility)
    {
        return false;
    }

    if (!Spec.m_pAbility->CanActivate(this))
    {
        return false;
    }

    Spec.m_bIsActive = true;
    Spec.m_pAbility->ActivateAbility(this);
    return true;
}

bool UAbilitySystemComponent::TryActivateAbilityByTag(const FGameplayTag& AbilityTag)
{
    for (int32 i = 0; i < m_GrantedAbilities.Num(); ++i)
    {
        FGameplayAbilitySpec& Spec = m_GrantedAbilities[i];
        if (Spec.m_pAbility && Spec.m_pAbility->m_AbilityTags.HasTag(AbilityTag))
        {
            return TryActivateAbility(i);
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void UAbilitySystemComponent::Tick(float DeltaTime)
{
    TickActiveEffects(DeltaTime);
}

void UAbilitySystemComponent::TickActiveEffects(float DeltaTime)
{
    bool bNeedRecalc = false;

    for (int32 i = m_ActiveEffects.Num() - 1; i >= 0; i--)
    {
        FActiveGameplayEffect& Active = m_ActiveEffects[i];
        UGameplayEffect* pSpec = Active.m_pSpec;
        if (!pSpec)
        {
            m_ActiveEffects.RemoveAtSwap(i);
            bNeedRecalc = true;
            continue;
        }

        if (pSpec->m_DurationType == EGameplayEffectDurationType::Infinite)
        {
            if (pSpec->m_Period > 0.f)
            {
                Active.m_PeriodTimer -= DeltaTime;
                // 디버거 중단·프레임 히치 등으로 DeltaTime이 Period의 여러 배가
                // 되면 논리적으로 여러 번 발동해야 한다 — if 한 번만 적용하면
                // 나머지가 이후 프레임들로 하나씩 뒤늦게 새어나온다.
                // pSpec->m_Period > 0.f로 이미 감싸여 있어 매 반복 양수만큼
                // 증가하므로 유한 횟수 안에 종료된다.
                while (Active.m_PeriodTimer <= 0.f)
                {
                    ApplyModifiersToBase(pSpec->m_Modifiers, Active.m_StackCount);
                    Active.m_PeriodTimer += pSpec->m_Period;
                    bNeedRecalc = true;
                }
            }
            continue;
        }

        Active.m_Duration -= DeltaTime;

        if (pSpec->m_Period > 0.f)
        {
            Active.m_PeriodTimer -= DeltaTime;
            while (Active.m_PeriodTimer <= 0.f)
            {
                ApplyModifiersToBase(pSpec->m_Modifiers, Active.m_StackCount);
                Active.m_PeriodTimer += pSpec->m_Period;
                bNeedRecalc = true;
            }
        }

        if (Active.m_Duration <= 0.f)
        {
            const TArray<FGameplayTag>& GrantedTags = pSpec->m_GrantedTags.GetTags();

            for (int32 t = 0; t < GrantedTags.Num(); t++)
            {
                m_ActiveTags.RemoveTag(GrantedTags[t]);
            }

            m_ActiveEffects.RemoveAtSwap(i);
            bNeedRecalc = true;
        }
    }

    if (bNeedRecalc)
    {
        RecalculateAttributes();
    }
}

// ---------------------------------------------------------------------------
// �Ӽ� ����
// RecalculateAttributes: ��Period Duration/Infinite ȿ���� �ٽ� ����.
//   1. ���� �Ӽ� CurrentValue = BaseValue �ʱ�ȭ
//   2. Add ���� �� Multiply ���� �� Override ����
// ---------------------------------------------------------------------------

void UAbilitySystemComponent::RecalculateAttributes()
{
    if (!m_pAttributeSet)
    {
        return;
    }

    m_pAttributeSet->ResetCurrentValues();

    for (int32 pass = 0; pass < 3; ++pass)
    {
        EGameplayModifierOperation OpFilter;

        if (pass == 0) 
        { 
            OpFilter = EGameplayModifierOperation::Add; 
        }

        else if (pass == 1) 
        { 
            OpFilter = EGameplayModifierOperation::Multiply; 
        }

        else 
        { 
            OpFilter = EGameplayModifierOperation::Override; 
        }

        for (int32 i = 0; i < m_ActiveEffects.Num(); i++)
        {
            const FActiveGameplayEffect& Active = m_ActiveEffects[i];
            UGameplayEffect* pSpec = Active.m_pSpec;
            if (!pSpec || pSpec->m_Period > 0.f)
            {
                continue;
            }

            const TArray<FGameplayEffectModifier>& Mods = pSpec->m_Modifiers;
            for (int32 m = 0; m < Mods.Num(); m++)
            {
                const FGameplayEffectModifier& Mod = Mods[m];
                if (Mod.m_Operation != OpFilter)
                {
                    continue;
                }

                FGameplayAttribute* Attr = m_pAttributeSet->GetAttribute(Mod.m_AttributeName);
                if (!Attr)
                {
                    continue;
                }

                float Val = Attr->GetCurrentValue();
                float Mag = Mod.m_Magnitude * Active.m_StackCount;

                if (OpFilter == EGameplayModifierOperation::Add)
                {
                    Val += Mag;
                }

                else if (OpFilter == EGameplayModifierOperation::Multiply)
                {
                    Val += Val * Mag;
                }

                else
                {
                    Val = Mod.m_Magnitude;
                }

                Attr->SetCurrentValue(Val);
            }
        }
    }
}

void UAbilitySystemComponent::ApplyModifiersToBase(const TArray<FGameplayEffectModifier>& Mods, int32 Stacks)
{
    if (!m_pAttributeSet)
    {
        return;
    }

    for (int32 i = 0; i < Mods.Num(); i++)
    {
        const FGameplayEffectModifier& Mod = Mods[i];
        FGameplayAttribute* Attr = m_pAttributeSet->GetAttribute(Mod.m_AttributeName);
        if (!Attr)
        {
            continue;
        }

        float Mag = Mod.m_Magnitude * Stacks;

        if (Mod.m_Operation == EGameplayModifierOperation::Add)
        {
            Attr->SetBaseValue(Attr->GetBaseValue() + Mag);
        }

        else if (Mod.m_Operation == EGameplayModifierOperation::Multiply)
        {
            float Base = Attr->GetBaseValue();

            Attr->SetBaseValue(Base + Base * Mag);
        }

        else
        {
            Attr->SetBaseValue(Mod.m_Magnitude);
        }
        Attr->SetCurrentValue(Attr->GetBaseValue());
    }
}