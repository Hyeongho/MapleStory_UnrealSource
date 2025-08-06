#pragma once

#include "../SmartPointer/TSharedPtr.h"
#include "../SmartPointer/TWeakPtr.h"
#include "../Cast/Cast.h"
#include "../SmartPointer/TSharedFromThis.h"
#include "../Container/TArray.h"
#include "Component.h"
#include "../String/FString.h"

class CGameObject : public TSharedFromThis<CGameObject>
{
private:
    FString m_Name;
    bool m_bActive;

    TArray<TSharedPtr<CComponent>> m_Components;

public:
    CGameObject();
    virtual ~CGameObject();

    void SetName(const FString& InName) 
    { 
        m_Name = InName; 
    }

    const FString& GetName() const
    {
        return m_Name;
    }

    void SetActive(bool bActive) 
    { 
        m_bActive = bActive; 
    }

    bool IsActive() const 
    { 
        return m_bActive; 
    }

    template <typename T, typename... Args>
    TSharedPtr<T> AddComponent(Args&&... args)
    {
        TSharedPtr<T> NewComp = MakeShared<T>(std::forward<Args>(args)...);
        NewComp->SetOwner(SharedFromThis());
        m_Components.Add(StaticPointerCast<CComponent>(NewComp));
        return NewComp;
    }

    template <typename T>
    TSharedPtr<T> GetComponent() const
    {
        for (const TSharedPtr<CComponent>& Comp : m_Components)
        {
            TSharedPtr<T> Casted = StaticPointerCast<T>(Comp);

            if (IsValid(Casted))
            {
                return Casted;
            }
        }

        return nullptr;
    }

    virtual void Update(float DeltaTime);
    virtual void Render();
};

