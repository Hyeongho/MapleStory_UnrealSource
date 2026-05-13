#include "EnginePCH.h"
#include "FRect.h"

FRect::FRect() : m_Left(0.f), m_Top(0.f), m_Right(0.f), m_Bottom(0.f)
{
}

FRect::FRect(float Left, float Top, float Right, float Bottom) : m_Left(Left), m_Top(Top), m_Right(Right), m_Bottom(Bottom)
{
}

FRect::FRect(FVector2D TopLeft, FVector2D BottomRight) : m_Left(TopLeft.m_X), m_Top(TopLeft.m_Y) , m_Right(BottomRight.m_X), m_Bottom(BottomRight.m_Y)
{
}

float FRect::Width() const 
{ 
	return m_Right - m_Left; 
}

float FRect::Height() const 
{ 
	return m_Bottom - m_Top; 
}

float FRect::Area() const 
{ 
	return Width() * Height(); 
}

FVector2D FRect::Center() const 
{ 
	return FVector2D((m_Left + m_Right) * 0.5f, (m_Top + m_Bottom) * 0.5f); 
}

bool FRect::Contains(FVector2D Point) const
{
	return Point.m_X >= m_Left && Point.m_X <= m_Right && Point.m_Y >= m_Top && Point.m_Y <= m_Bottom;
}

bool FRect::Overlaps(const FRect& Other) const
{
	return !(Other.m_Left > m_Right || Other.m_Right  < m_Left || Other.m_Top  > m_Bottom || Other.m_Bottom < m_Top);
}

FRect FRect::Intersect(const FRect& Other) const
{
	FRect Result;

	Result.m_Left = FMath::Max(m_Left, Other.m_Left);
	Result.m_Top = FMath::Max(m_Top, Other.m_Top);
	Result.m_Right = FMath::Min(m_Right, Other.m_Right);
	Result.m_Bottom = FMath::Min(m_Bottom, Other.m_Bottom);

	return Result;
}

bool FRect::operator==(const FRect& Other) const
{
	return FMath::IsNearlyEqual(m_Left, Other.m_Left) && FMath::IsNearlyEqual(m_Top, Other.m_Top) && FMath::IsNearlyEqual(m_Right, Other.m_Right) && FMath::IsNearlyEqual(m_Bottom, Other.m_Bottom);
}

bool FRect::operator!=(const FRect& Other) const
{
	return !(*this == Other);
}