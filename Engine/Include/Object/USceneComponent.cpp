#include "EnginePCH.h"
#include "Object/USceneComponent.h"

USceneComponent::USceneComponent() : m_pAttachParent(nullptr) 
{
}

USceneComponent::~USceneComponent() 
{
}

FTransform2D USceneComponent::GetWorldTransform() const
{
    if (m_pAttachParent)
    {
        return m_pAttachParent->GetWorldTransform();
    }

    return m_RelativeTransform;
}

void USceneComponent::SetAttachParent(USceneComponent* Parent)
{
    m_pAttachParent = Parent;

    if (Parent)
    {
        Parent->m_Children.Add(this);
    }
}
