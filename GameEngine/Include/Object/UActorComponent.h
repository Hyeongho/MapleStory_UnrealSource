#pragma once
#include "Object/UObject.h"

class AActor;

class UActorComponent : public UObject
{
    DECLARE_CLASS(UActorComponent, UObject)
public:
    UActorComponent();
    virtual ~UActorComponent() override;

    AActor* GetOwner() const 
    { 
        return m_pOwner; 
    }

    void    SetOwner(AActor* Owner) 
    { 
        m_pOwner = Owner; 
    }

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay() override;

private:
    AActor* m_pOwner;
};