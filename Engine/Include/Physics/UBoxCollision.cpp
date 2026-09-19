#include "EnginePCH.h"
#include "Physics/UBoxCollision.h"

UBoxCollision::UBoxCollision() = default;
UBoxCollision::~UBoxCollision() = default;

void UBoxCollision::SetBoxExtent(const FVector2D& HalfExtent)
{
	m_BoxExtent = FVector2D(FMath::Max(0.0f, HalfExtent.m_X), FMath::Max(0.0f, HalfExtent.m_Y));
}

FVector2D UBoxCollision::GetScaledBoxExtent() const
{
	const FVector2D Scale = GetWorldTransform().m_Scale;
	return FVector2D(m_BoxExtent.m_X * FMath::Abs(Scale.m_X), m_BoxExtent.m_Y * FMath::Abs(Scale.m_Y));
}

ECollisionShape UBoxCollision::GetShapeType() const
{
	return ECollisionShape::Box;
}

FRect UBoxCollision::GetWorldBounds() const
{
	return GetWorldBox().GetBounds();
}

FOrientedBox2D UBoxCollision::GetWorldBox() const
{
	return FOrientedBox2D(GetWorldCenter(), GetScaledBoxExtent(), GetWorldTransform().m_Rotation);
}
