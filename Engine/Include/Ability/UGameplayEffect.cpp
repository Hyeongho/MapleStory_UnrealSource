#include "EnginePCH.h"
#include "Ability/UGameplayEffect.h"

UGameplayEffect::UGameplayEffect() = default;

void UGameplayEffect::AddModifier(const FName& Attr, EGameplayModifierOperation Op, float Mag)
{
    FGameplayEffectModifier Mod;
    Mod.m_AttributeName = Attr;
    Mod.m_Operation = Op;
    Mod.m_Magnitude = Mag;
    m_Modifiers.Add(Mod);
}

void UGameplayEffect::AddGrantedTag(const FGameplayTag& Tag)
{
    m_GrantedTags.AddTag(Tag);
}