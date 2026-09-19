#include "EnginePCH.h"
#include "Physics/UClimbableComponent.h"

UClimbableComponent::UClimbableComponent()
{
	SetBoxExtent(FVector2D(6.0f, 32.0f));
	SetCollisionObjectType(ECollisionChannel::Trigger);
	SetCollisionMask(CollisionChannelMask(ECollisionChannel::Player));
}

UClimbableComponent::~UClimbableComponent() = default;

void UClimbableComponent::SetClimbableType(EClimbableType Type)
{
	m_Type = Type;
}

EClimbableType UClimbableComponent::GetClimbableType() const
{
	return m_Type;
}
