#pragma once
#include "EnginePCH.h"
#include "Object/UObject.h"
#include "Ability/FGameplayAttribute.h"
#include "Core/Containers/TMap.h"
#include "Core/String/FName.h"

class UAttributeSet : public UObject
{
    DECLARE_CLASS(UAttributeSet, UObject)
public:
    UAttributeSet();
    virtual ~UAttributeSet() override = default;

    void InitAttribute(const FName& Name, float Base, float Min = 0.f, float Max = 3.4e38f);

    FGameplayAttribute* GetAttribute(const FName& Name);
    const FGameplayAttribute* GetAttribute(const FName& Name) const;
    bool  HasAttribute(const FName& Name) const;
    float GetCurrentValue(const FName& Name) const;
    float GetBaseValue(const FName& Name)    const;

    void ResetCurrentValues();

private:
    TMap<FName, FGameplayAttribute> m_Attributes;
};