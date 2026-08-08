#pragma once
#include "EnginePCH.h"

class UGameplayEffect;
class UGameplayAbility;

enum class EGameplayEffectDurationType : uint8
{
    Instant,
    Duration,
    Infinite,
};

enum class EGameplayModifierOperation : uint8
{
    Add,
    Multiply,
    Override,
};

struct FGameplayEffectModifier
{
    FName m_AttributeName;
    EGameplayModifierOperation m_Operation = EGameplayModifierOperation::Add;
    float m_Magnitude = 0.f;
};

struct FActiveGameplayEffect
{
    UGameplayEffect* m_pSpec = nullptr;
    float m_Duration = 0.f;
    float m_PeriodTimer = 0.f;
    int32 m_StackCount = 1;
};

struct FGameplayAbilitySpec
{
    UGameplayAbility* m_pAbility = nullptr;
    int32 m_Level = 1;
    bool m_bIsActive = false;
};
