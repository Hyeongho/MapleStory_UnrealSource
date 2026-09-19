#include "EnginePCH.h"
#include "Physics/UCircleCollision.h"

UCircleCollision::UCircleCollision() = default;
UCircleCollision::~UCircleCollision() = default;

void UCircleCollision::SetSphereRadius(float Radius)
{
	m_Radius = FMath::Max(0.0f, Radius);
}

float UCircleCollision::GetWorldRadius() const
{
	const FVector2D Scale = GetWorldTransform().m_Scale;
	return m_Radius * FMath::Max(FMath::Abs(Scale.m_X), FMath::Abs(Scale.m_Y));
}

ECollisionShape UCircleCollision::GetShapeType() const
{
	return ECollisionShape::Circle;
}

FRect UCircleCollision::GetWorldBounds() const
{
	const FVector2D Center = GetWorldCenter(), Extent(GetWorldRadius());
	return FRect(Center - Extent, Center + Extent);
}