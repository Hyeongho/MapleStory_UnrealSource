#include "EnginePCH.h"
#include "FTransform2D.h"

FTransform2D::FTransform2D() : m_Location(0.f, 0.f), m_Rotation(0.f), m_Scale(1.f, 1.f)
{
}

FTransform2D::FTransform2D(FVector2D Location, float Rotation, FVector2D Scale) : m_Location(Location), m_Rotation(Rotation), m_Scale(Scale)
{
}

FTransform2D FTransform2D::Identity()
{
	return FTransform2D(FVector2D(0.f, 0.f), 0.f, FVector2D(1.f, 1.f));
}

FVector2D FTransform2D::TransformPoint(FVector2D Point) const
{
	// 1. Scale
	float SX = Point.m_X * m_Scale.m_X;
	float SY = Point.m_Y * m_Scale.m_Y;
	// 2. Rotate
	float C = FMath::Cos(m_Rotation);
	float S = FMath::Sin(m_Rotation);
	float RX = SX * C - SY * S;
	float RY = SX * S + SY * C;
	// 3. Translate
	return FVector2D(RX + m_Location.m_X, RY + m_Location.m_Y);
}

FVector2D FTransform2D::TransformDirection(FVector2D Dir) const
{
	float SX = Dir.m_X * m_Scale.m_X;
	float SY = Dir.m_Y * m_Scale.m_Y;
	float C = FMath::Cos(m_Rotation);
	float S = FMath::Sin(m_Rotation);

	return FVector2D(SX * C - SY * S, SX * S + SY * C);
}

FTransform2D FTransform2D::Inverse() const
{
	FTransform2D Inv;

	Inv.m_Scale.m_X = (FMath::Abs(m_Scale.m_X) > FMath::SMALL_NUMBER) ? 1.f / m_Scale.m_X : 0.f;
	Inv.m_Scale.m_Y = (FMath::Abs(m_Scale.m_Y) > FMath::SMALL_NUMBER) ? 1.f / m_Scale.m_Y : 0.f;
	Inv.m_Rotation = -m_Rotation;

	float C = FMath::Cos(-m_Rotation);
	float S = FMath::Sin(-m_Rotation);
	float IX = -m_Location.m_X * Inv.m_Scale.m_X;
	float IY = -m_Location.m_Y * Inv.m_Scale.m_Y;
	Inv.m_Location.m_X = IX * C - IY * S;
	Inv.m_Location.m_Y = IX * S + IY * C;

	return Inv;
}

bool FTransform2D::operator==(const FTransform2D& Other) const
{
	return m_Location == Other.m_Location && FMath::IsNearlyEqual(m_Rotation, Other.m_Rotation) && m_Scale == Other.m_Scale;
}

bool FTransform2D::operator!=(const FTransform2D& Other) const
{
	return !(*this == Other);
}
