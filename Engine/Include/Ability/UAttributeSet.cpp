#include "EnginePCH.h"
#include "Ability/UAttributeSet.h"

UAttributeSet::UAttributeSet() = default;

void UAttributeSet::InitAttribute(const FName& Name, float Base, float Min, float Max)
{
    FGameplayAttribute Attr(Base, Min, Max);
    m_Attributes.Add(Name, Attr);
}

FGameplayAttribute* UAttributeSet::GetAttribute(const FName& Name)
{
    return m_Attributes.Find(Name);
}

const FGameplayAttribute* UAttributeSet::GetAttribute(const FName& Name) const
{
    return m_Attributes.Find(Name);
}

bool UAttributeSet::HasAttribute(const FName& Name) const
{
    return m_Attributes.Contains(Name);
}

float UAttributeSet::GetCurrentValue(const FName& Name) const
{
    const FGameplayAttribute* Attr = m_Attributes.Find(Name);
    return Attr ? Attr->GetCurrentValue() : 0.f;
}

float UAttributeSet::GetBaseValue(const FName& Name) const
{
    const FGameplayAttribute* Attr = m_Attributes.Find(Name);
    return Attr ? Attr->GetBaseValue() : 0.f;
}

void UAttributeSet::ResetCurrentValues()
{
    for (auto& Bucket : m_Attributes)
    {
        Bucket.Value.SetCurrentValue(Bucket.Value.GetBaseValue());
    }
}
