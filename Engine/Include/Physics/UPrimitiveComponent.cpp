#include "EnginePCH.h"
#include "Physics/UPrimitiveComponent.h"
#include "Physics/UCircleCollision.h"

UPrimitiveComponent::UPrimitiveComponent() = default;
UPrimitiveComponent::~UPrimitiveComponent() = default;

FVector2D UPrimitiveComponent::GetWorldCenter() const
{
	const FTransform2D Transform = GetWorldTransform();
	return Transform.m_Location + FVector2D(m_CenterOffset.m_X * Transform.m_Scale.m_X, m_CenterOffset.m_Y * Transform.m_Scale.m_Y);
}

void UPrimitiveComponent::SetCenterOffset(const FVector2D& Offset)
{
	m_CenterOffset = Offset;
}

void UPrimitiveComponent::SetCollisionEnabled(bool bEnabled)
{
	m_bCollisionEnabled = bEnabled;
}

bool UPrimitiveComponent::IsCollisionEnabled() const
{
	return m_bCollisionEnabled;
}

void UPrimitiveComponent::SetCollisionObjectType(ECollisionChannel Channel)
{
	if (CollisionChannelMask(Channel))
	{
		m_ObjectType = Channel;
	}
}

ECollisionChannel UPrimitiveComponent::GetCollisionObjectType() const
{
	return m_ObjectType;
}

void UPrimitiveComponent::SetCollisionMask(uint32 Mask)
{
	m_CollisionMask = Mask & AllCollisionChannels;
}

uint32 UPrimitiveComponent::GetCollisionMask() const
{
	return m_CollisionMask;
}

bool UPrimitiveComponent::CanCollideWith(const UPrimitiveComponent& Other) const
{
	return m_bCollisionEnabled && Other.m_bCollisionEnabled && (m_CollisionMask & CollisionChannelMask(Other.m_ObjectType)) && (Other.m_CollisionMask & CollisionChannelMask(m_ObjectType));
}

bool UPrimitiveComponent::Overlaps(const UPrimitiveComponent& Other) const
{
	if (!CanCollideWith(Other))
	{
		return false;
	}

	if (GetShapeType() == ECollisionShape::Box && Other.GetShapeType() == ECollisionShape::Box)
	{
		return GetWorldBounds().Overlaps(Other.GetWorldBounds());
	}

	if (GetShapeType() == ECollisionShape::Circle && Other.GetShapeType() == ECollisionShape::Circle)
	{
		const float Radius = static_cast<const UCircleCollision*>(this)->GetWorldRadius() + static_cast<const UCircleCollision&>(Other).GetWorldRadius();

		return FVector2D::DistanceSquared(GetWorldCenter(), Other.GetWorldCenter()) <= Radius * Radius;
	}

	const UCircleCollision& Circle = GetShapeType() == ECollisionShape::Circle ? static_cast<const UCircleCollision&>(*this) : static_cast<const UCircleCollision&>(Other);

	const FRect Box = GetShapeType() == ECollisionShape::Box ? GetWorldBounds() : Other.GetWorldBounds();

	const FVector2D Center = Circle.GetWorldCenter();
	const FVector2D Closest(FMath::Clamp(Center.m_X, Box.m_Left, Box.m_Right), FMath::Clamp(Center.m_Y, Box.m_Top, Box.m_Bottom));

	const float Radius = Circle.GetWorldRadius();

	return FVector2D::DistanceSquared(Center, Closest) <= Radius * Radius;
}