#pragma once
#include "EnginePCH.h"
#include "Core/Math/FMath.h"

struct FGameplayAttribute
{
    float m_BaseValue    = 0.f;
    float m_CurrentValue = 0.f;
    float m_MinValue     = 0.f;
    float m_MaxValue     = 3.4e38f;

    FGameplayAttribute() = default;

    FGameplayAttribute(float Base, float Min = 0.f, float Max = 3.4e38f)
        : m_BaseValue(Base)
        , m_CurrentValue(Base)
        , m_MinValue(Min)
        , m_MaxValue(Max)
    {
    }

    void SetBaseValue(float NewBase)
    {
        m_BaseValue = FMath::Clamp(NewBase, m_MinValue, m_MaxValue);
    }

    void SetCurrentValue(float NewCurrent)
    {
        m_CurrentValue = FMath::Clamp(NewCurrent, m_MinValue, m_MaxValue);
    }

    float GetBaseValue()    const { return m_BaseValue; }
    float GetCurrentValue() const { return m_CurrentValue; }
};
