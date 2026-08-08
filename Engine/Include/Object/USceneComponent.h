#pragma once
#include "Object/UActorComponent.h"
#include "Core/Math/FTransform2D.h"
#include "Core/Containers/TArray.h"

class USceneComponent : public UActorComponent
{
    DECLARE_CLASS(USceneComponent, UActorComponent)
public:
    USceneComponent();
    virtual ~USceneComponent() override;

    const FTransform2D& GetRelativeTransform() const 
    { 
        return m_RelativeTransform; 
    }

    void SetRelativeTransform(const FTransform2D& T) 
    { 
        m_RelativeTransform = T; 
    }

    FTransform2D GetWorldTransform() const;

    void SetAttachParent(USceneComponent* Parent);

    USceneComponent* GetAttachParent() const 
    { 
        return m_pAttachParent;
    }

private:
    FTransform2D              m_RelativeTransform;
    USceneComponent* m_pAttachParent;
    TArray<USceneComponent*>  m_Children;
};
